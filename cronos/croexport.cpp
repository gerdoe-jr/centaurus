#include "croexport.h"
#include <win32util.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* CroExport */

template<CroExportFormat F>
CroExport<F>::CroExport(CroBank* bank)
    : CroReader(bank), m_pOut(NULL)
{
}

template<CroExportFormat F>
CroExport<F>::~CroExport()
{
}

template<CroExportFormat F>
void CroExport<F>::SetIdentOutput(CroIdent ident, CroSync* out)
{
    m_Outputs[ident] = out;
}

template<CroExportFormat F>
std::string CroExport<F>::StringValue()
{
    uint8_t* value = m_Parser.Value();
    cronos_size size = m_Parser.ValueSize();
    CroType type = m_Parser.ValueType();

    CroBuffer out;
    CroStream str = CroStream(out);
    
    std::string text;
    char szNum[32] = { 0 };
    int iYear = 0, iMon = 0, iDay = 0;
    int iHour = 0, iMin = 0;

    switch (type)
    {
    case CroType::Ident:
        text = std::to_string(m_Parser.RecordId());
        
        out.Alloc(text.size());
        str.Write((const uint8_t*)text.c_str(), text.size());
        break;
    case CroType::Integer:
        if (!size) return "";
        out.Alloc(size);

        for (cronos_size i = 0; i < size; i++)
        {
            if (isdigit(value[i]))
                str.Write<uint8_t>(value[i]);
        }
        break;
    case CroType::String:
        if (!size) return "";
        out.Alloc(size);

        for (cronos_size i = 0; i < size; i++)
        {
            uint8_t c = value[i];
            if (!c) break;

            str.Write<uint8_t>(value[i]);
        }
        break;
    case CroType::VocString:
        text = "VocString(" + std::string((const char*)value, size) + ")";

        out.Alloc(text.size());
        str.Write((const uint8_t*)text.c_str(), text.size());
        break;
    case CroType::Date:
        out.Alloc(12);

        memcpy(szNum, value, 7);
        szNum[7] = '\0';
        sscanf_s(szNum, "%03d%02d%02d", &iYear, &iMon, &iDay);

        sprintf_s(szNum, "%02d.%02d.%04d", iDay, iMon, iYear + 1900);
        str.Write((const uint8_t*)szNum, strlen(szNum));
        break;
    case CroType::Time:
        out.Alloc(12);

        memcpy(szNum, value, 4);
        szNum[4] = '\0';
        sscanf_s(szNum, "%02d%02d", &iHour, &iMin);

        sprintf(szNum, "%02d:%02d", iHour, iMin);
        str.Write((const uint8_t*)szNum, 5);
        break;
    default:
        return "CroType(" + std::to_string((unsigned)type) + ")";
    }

    return std::string((const char*)out.GetData(), str.GetPosition());
}

template<CroExportFormat F>
CroSync* CroExport<F>::GetExportOutput() const
{
    return m_pOut;
}

template<CroExportFormat F>
CroExportFormat CroExport<F>::GetExportFormat() const
{
    return F;
}

template<CroExportFormat F>
void CroExport<F>::OnRecord()
{
    uint32_t index = m_Parser.IdentBase()->m_BaseIndex;
    auto out = m_Outputs.find(index);

    if (!m_pOut)
    {
        m_pOut = out == m_Outputs.end() ? NULL : m_Outputs[index];
    }

    CroReader::OnRecord();
}

template<CroExportFormat F>
void CroExport<F>::OnRecordEnd()
{
    m_pOut = NULL;
    CroReader::OnRecordEnd();
}

/* CroExportRaw */

CroExportRaw::CroExportRaw(CroBank* bank)
    : CroExport<CroExportFormat::Raw>(bank)
{
}

void CroExportRaw::OnRecord()
{
    CroExport::OnRecord();
}

void CroExportRaw::OnValue()
{
    CroExport::OnValue();

    m_pOut->Write(m_Parser.Value(), m_Parser.ValueSize());
}

/* CroExportList */

CroExportList::CroExportList(CroBank* bank, cronos_idx burst)
    : CroExportRaw(bank), m_pRecord(NULL)
{
    CroFile* file = bank->File(CROFILE_BANK);
    m_Output.InitSync(file->GetDefaultBlockSize() * burst);
}

void CroExportList::Reset()
{
    m_Parser.Reset();
    m_Output.Flush();

    m_List.clear();
}

void CroExportList::OnRecord()
{
    cronos_id id = m_Parser.RecordId();

    m_List.emplace(id, CroExportRecord {});
    m_pRecord = &m_List[id];

    m_pOut = &m_Output;
    CroExportRaw::OnRecord();
}

void CroExportList::OnValue()
{
    CroExport::OnValue();

    CroBuffer value;
    if (m_Parser.ValueSize())
    {
        value.Copy(m_Parser.Value(), m_Parser.ValueSize());
    }

    m_pRecord->push_back(value);
}

/* CroExportCSV */

CroExportCSV::CroExportCSV(CroBank* bank)
    : CroExport<CroExportFormat::CSV>(bank)
{
}

void CroExportCSV::OnRecord()
{
    CroExport::OnRecord();
}

void CroExportCSV::OnRecordEnd()
{
    if (m_pOut)
    {
        uint8_t csvNewLine[] = { '\r', '\n' };

        m_pOut->Write(csvNewLine, 2);
    }

    CroExport::OnRecordEnd();
}

void CroExportCSV::OnValue()
{
    CroExport::OnValue();

    uint8_t* value = m_Parser.Value();
    cronos_size size = m_Parser.ValueSize();

    uint8_t csvQuote = '"';

    std::string str = StringValue();
    bool quoted = str.empty() ? true
        : str.find(csvQuote) != std::string::npos;

    if (quoted)
        m_pOut->Write(&csvQuote, 1);

    std::string text = WcharToText(AnsiToWchar(
        str, m_pBank->GetTextCodePage()));
    for (cronos_size i = 0; i < text.size(); i++)
    {
        if (text[i] == csvQuote)
            m_pOut->Write(&csvQuote, 1);
        m_pOut->Write((const uint8_t*)&text[i], 1);
    }

    if (quoted)
        m_pOut->Write(&csvQuote, 1);
}

void CroExportCSV::OnValueNext()
{
    CroExport::OnValueNext();
    
    if (m_State == CroValue_Read)
    {
        uint8_t csvComma = ',';
        m_pOut->Write(&csvComma, 1);
    }
}
