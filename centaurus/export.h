#ifndef __CENTAURUS_EXPORT_H
#define __CENTAURUS_EXPORT_H

#include "centaurus.h"
#include "cronos_api.h"
#include <croexport.h>
#include <json_file.h>
#include <memory>

class CentaurusExport : public CronosAPI, public ICentaurusExport
{
public:
    CentaurusExport();
    virtual ~CentaurusExport();

    const std::wstring& ExportPath() const override;
    ExportFormat GetExportFormat() const override;
    void SetExportFormat(ExportFormat fmt) override;
    void SyncBankJson() override;

    void PrepareDirs();

    void RunTask() override;
    void Release() override;

    void Export();
private:
    std::wstring m_ExportPath;
    ExportFormat m_ExportFormat;

    nlohmann::json m_BankJson;
    std::unique_ptr<ICroExport> m_Export;
};

#endif