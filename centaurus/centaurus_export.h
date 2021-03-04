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

class CsvBuffer : public CroBuffer
{
public:
    CsvBuffer();
    CsvBuffer(unsigned columns);

    void Write(const std::string& column);
    void Flush(FILE* fCsv);
private:
    unsigned m_uIndex;
    unsigned m_uColumns;
    cronos_off m_CsvOffset;
};

struct ExportOutput {
    FILE* m_fCsv;
    CsvBuffer m_CsvBuffer;
};

#include <map>

class CentaurusExport : public CentaurusTask, public ICentaurusExport
{
public:
    CentaurusExport(ICentaurusBank* bank, const std::wstring& path);
    virtual ~CentaurusExport();

    void PrepareDirs();
    void OpenExport();
    void CloseExport();
    void FlushExport();
    void SaveExportRecord(CroBuffer& record, uint32_t id);

    void Run() override;

    std::wstring GetFileName(CroFile* file);
    void ExportCroFile(CroFile* file);

    ExportRecord ReadExportRecord(CroFile* file, CroEntry& entry);
    CroBuffer ReadFileRecord(CroFile* file, ExportRecord& record);

    //CroBuffer GetRecord(CroFile* file, CroEntry& entry, CroRecordTable* dat);
    ICentaurusBank* TargetBank() override;
    const std::wstring& ExportPath() const override;
    void ReadRecord(CroFile* file, uint32_t id, CroBuffer& out) override;
private:
    ICentaurusBank* m_pBank;
    std::wstring m_ExportPath;
    std::map<cronos_idx, ExportOutput> m_Export;
};
#endif

#endif