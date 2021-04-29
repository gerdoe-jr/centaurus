#include "croexport.h"

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
