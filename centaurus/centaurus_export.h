#ifndef __CENTAURUS_EXPORT_H
#define __CENTAURUS_EXPORT_H

#ifdef CENTAURUS_INTERNAL
#include "centaurus.h"
#include "centaurus_task.h"
#include <crotable.h>
#include <string>

struct FileBlock {
    cronos_off m_DataPos;
    cronos_size m_DataSize;
};

class ExportRecord
{
public:
    ExportRecord(CroEntry& entry)
    {
        m_RecordId = entry.Id();
        m_RecordFlags = entry.EntryFlags();
    }

    inline void AddBlock(cronos_off off, cronos_size size)
    {
        m_Blocks.emplace_back(off, size);
    }

    cronos_id m_RecordId;
    cronos_flags m_RecordFlags;
    std::vector<FileBlock> m_Blocks;
};

class ExportBuffer : public CroBuffer
{
public:
    ExportBuffer();
    ExportBuffer(ExportFormat fmt, unsigned columns);

    void ReserveSize(cronos_size size);

    void WriteCSV(const std::string& column);
    void WriteJSON(const std::string& column);

    void Write(const std::string& column);
    void Flush(FILE* fCsv);
private:
    ExportFormat m_Format;
    unsigned m_uIndex;
    unsigned m_uColumns;

    cronos_size m_Reserve;
    cronos_off m_TextOffset;
};

struct ExportOutput {
    FILE* m_fExport;
    ExportBuffer m_Buffer;
};

#include <map>
#include <json.hpp>

class CentaurusExport : public CentaurusTask, public ICentaurusExport
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
    void ExportCroFile(CroFile* file);

    ExportRecord ReadExportRecord(CroFile* file, CroEntry& entry);
    CroBuffer ReadFileRecord(CroFile* file, ExportRecord& record);

    //CroBuffer GetRecord(CroFile* file, CroEntry& entry, CroRecordTable* dat);
    ICentaurusBank* TargetBank() override;
    const std::wstring& ExportPath() const override;
    ExportFormat GetExportFormat() const override;
    void SetExportFormat(ExportFormat fmt) override;
    void ReadRecord(CroFile* file, uint32_t id, CroBuffer& out) override;
    void SyncBankJson() override;

    inline void SetTargetBank(ICentaurusBank* bank) { m_pBank = bank; }
protected:
    ICentaurusBank* m_pBank;
    std::wstring m_ExportPath;
    ExportFormat m_ExportFormat;
private:
    std::map<cronos_idx, ExportOutput> m_Export;
    nlohmann::json m_BankJson;
};
#endif

#endif