#include "croexport.h"

/* CroExport */

template<CroExportFormat F>
CroExport<F>::CroExport(CroBank* bank)
    : CroReader(bank), m_pOut(NULL)
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

    m_pOut = out == m_Outputs.end() ? NULL : m_Outputs[index];
    if (m_pOut)
    {
        CroReader::OnRecord();
    }
}

/* CroExportRaw */

CroExportRaw::CroExportRaw(CroBank* bank, CroSync* out)
    : CroExport<CroExportFormat::Raw>(bank)
{
}

void CroExportRaw::OnRecord()
{
    CroExport::OnRecord();
}

void CroExportRaw::OnValue()
{
    m_pOut->Write(m_Parser.Value(), m_Parser.ValueSize());
}
