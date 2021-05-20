#ifndef __CENTAURUS_EXPORT_H
#define __CENTAURUS_EXPORT_H

#include <centaurus_api.h>

#include <croexport.h>
#include <json_file.h>
#include <memory>

#include "cronos_api.h"

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
    ExportFormat m_ExportFormat;

    std::wstring m_ExportPath;
    std::wstring m_FilePath;

    nlohmann::json m_BankJson;
    std::unique_ptr<ICroExport> m_Export;
};

#endif