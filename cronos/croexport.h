#ifndef __CROEXPORT_H
#define __CROEXPORT_H

#include "croreader.h"

enum class CroExportFormat {
    Raw,
    CSV,
    JSON
};

template<CroExportFormat F> class CroExport;

class ICroExport
{
public:
    virtual ~ICroExport() {}

    virtual void SetIdentOutput(CroIdent ident, CroSync* out) = 0;

    virtual CroSync* GetExportOutput() const = 0;
    virtual CroExportFormat GetExportFormat() const = 0;

    template<CroExportFormat F> inline CroExport<F>* GetExport()
    {
        return dynamic_cast<CroExport<F>*>(this);
    }
};

template<CroExportFormat F>
class CroExport : public ICroExport, public CroReader
{
public:
    CroExport(CroBank* bank);
    virtual ~CroExport();
    
    void SetIdentOutput(CroIdent ident, CroSync* out) override;

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
    CroExportRaw(CroBank* bank);
protected:
    void OnRecord() override;
    void OnValue() override;
};

#endif