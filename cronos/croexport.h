#ifndef __CROEXPORT_H
#define __CROEXPORT_H

#include "croreader.h"
#include "crobuffer.h"
#include <vector>
#include <map>

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
    virtual std::string StringValue() = 0;

    virtual CroSync* GetExportOutput() const = 0;
    virtual CroExportFormat GetExportFormat() const = 0;

    inline CroReader* GetReader()
    {
        return dynamic_cast<CroReader*>(this);
    }

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
    std::string StringValue() override;

    virtual CroSync* GetExportOutput() const;
    virtual CroExportFormat GetExportFormat() const;
protected:
    virtual void OnRecord();
    virtual void OnRecordEnd();

    CroSync* m_pOut;
private:
    std::map<CroIdent, CroSync*> m_Outputs;
};

class CroExportRaw : public CroExport<CroExportFormat::Raw>
{
public:
    CroExportRaw(CroBank* bank);
protected:
    virtual void OnRecord();
    virtual void OnValue();
};

using CroExportRecord = std::vector<CroBuffer>;
using CroExportIter = std::map<cronos_id, CroExportRecord>::iterator;

class CroExportList : public CroExportRaw
{
public:
    CroExportList(CroBank* bank, cronos_idx burst);

    inline CroExportIter ExportStart() { return m_List.begin(); }
    inline CroExportIter ExportEnd() { return m_List.end(); }

    void Reset();
protected:
    void OnRecord() override;
    void OnValue() override;
private:
    CroSync m_Output;
    std::map<cronos_id, CroExportRecord> m_List;

    CroExportRecord* m_pRecord;
};

class CroExportCSV : public CroExport<CroExportFormat::CSV>
{
public:
    CroExportCSV(CroBank* bank);
protected:
    virtual void OnRecord();
    virtual void OnRecordEnd();

    virtual void OnValue();
    virtual void OnValueNext();
};

#endif