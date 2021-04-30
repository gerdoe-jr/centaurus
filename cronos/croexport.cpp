#include "croexport.h"
#include <win32util.h>

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
    if (!size) return "";

    CroBuffer out;
    out.Alloc(size);

    std::string ident;
    CroStream str = CroStream(out);

    switch (m_Parser.ValueType())
    {
    case CroType::Ident:
        ident = std::to_string(m_Parser.RecordId());
        out.Alloc(ident.size());
        
        str.Write((const uint8_t*)ident.c_str(), ident.size());
        break;
    case CroType::Integer:
        for (cronos_size i = 0; i < size; i++)
        {
            if (isdigit(value[i]))
                str.Write<uint8_t>(value[i]);
        }
        break;
    default:
        for (cronos_size i = 0; i < size; i++)
        {
            uint8_t c = value[i];
            if (!c) break;

            str.Write<uint8_t>(value[i]);
        }
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

    if (m_pOut)
    {
        uint8_t csvNewLine[] = { '\r', '\n' };

        m_pOut->Write(csvNewLine, 2);
    }
}

void CroExportCSV::OnValue()
{
    CroExport::OnValue();

    uint8_t* value = m_Parser.Value();
    cronos_size size = m_Parser.ValueSize();
    CroType type = m_Parser.ValueType();

    uint8_t csvQuote = '"';
    uint8_t csvComma = ',';

    std::string str = StringValue();

    if (type == CroType::Ident || type == CroType::Integer)
        m_pOut->Write((const uint8_t*)str.c_str(), str.size());
    else
    {
        std::string text = WcharToText(AnsiToWchar(
            str, m_pBank->GetTextCodePage()));

        m_pOut->Write(&csvQuote, 1);
        for (cronos_size i = 0; i < text.size(); i++)
        {
            if (text[i] == csvQuote)
                m_pOut->Write(&csvQuote, 1);
            m_pOut->Write((const uint8_t*)&text[i], 1);
        }
        m_pOut->Write(&csvQuote, 1);
    }

    if (!m_Parser.IsLastField())
        m_pOut->Write(&csvComma, 1);
}
