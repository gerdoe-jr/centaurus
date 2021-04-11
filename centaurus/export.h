#ifndef __CENTAURUS_EXPORT_H
#define __CENTAURUS_EXPORT_H

#ifdef CENTAURUS_INTERNAL
#include "centaurus.h"
#include "cronos_api.h"
#include "task.h"
#include <crotable.h>
#include <string>

class ExportBuffer : public CroBuffer
{
public:
    ExportBuffer();
    ExportBuffer(ExportFormat fmt, unsigned columns);
    ~ExportBuffer();

    void SetExportFilePath(const std::wstring& path);
    void ReserveSize(cronos_size size);
    inline const std::wstring& ExportFilePath() { return m_ExportFile; }

    void WriteCSV(const std::string& column);
    void WriteJSON(const std::string& column);

    void Write(const std::string& column);
    void Flush();
    inline unsigned RecordCount() const { return m_uRecordCount; }
private:
    ExportFormat m_Format;
    unsigned m_uIndex;
    unsigned m_uColumns;

    cronos_size m_Reserve;
    cronos_off m_TextOffset;

    unsigned m_uRecordCount;
    std::wstring m_ExportFile;
};

#include <map>
#include <json.hpp>

class CentaurusExport : public CronosAPI, public ICentaurusExport
{
public:
    CentaurusExport();
    virtual ~CentaurusExport();

    std::wstring GetFileName(CroFile* file);

    void PrepareDirs();
    void SaveExportRecord(CroBuffer& record, uint32_t id);

    void RunTask() override;
    void Release() override;

    void Export();
    void ExportHeaders();
    
    centaurus_size GetMemoryUsage() override;
    void OnExportRecord(CroBuffer& record, uint32_t id);
    void FlushBuffers();

    const std::wstring& ExportPath() const override;
    ExportFormat GetExportFormat() const override;
    void SetExportFormat(ExportFormat fmt) override;

    void SyncBankJson() override;
private:
    std::wstring m_ExportPath;
    ExportFormat m_ExportFormat;

    std::map<cronos_idx, std::unique_ptr<ExportBuffer>> m_Export;
    nlohmann::json m_BankJson;
    centaurus_size m_TableLimit;
    centaurus_size m_BaseLimit;
};
#endif

#endif