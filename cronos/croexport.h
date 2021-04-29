#ifndef __CROEXPORT_H
#define __CROEXPORT_H

#include "croreader.h"

enum class CroExportFormat {
    Raw,
    CSV,
    JSON
};

template<CroExportFormat F>
class CroExport : public CroReader
{
public:
    CroExport(CroBank* bank);
    
    void SetIdentOutput(CroIdent ident, CroSync* out);

    virtual CroSync* GetExportOutput() const;
    virtual CroExportFormat GetExportFormat() const;
protected:
    virtual void OnRecord();

    CroSync* m_pOut;
private:
    std::map<CroIdent, CroSync*> m_Outputs;
};

class CroExportRaw : public CroExport<CroExportFormat::Raw>
{
public:
    CroExportRaw(CroBank* bank, CroSync* out);
protected:
    void OnRecord() override;
    void OnValue() override;
};

#endif