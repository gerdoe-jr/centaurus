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

class CentaurusExport : public CentaurusTask
{
public:
    CentaurusExport(ICentaurusBank* bank, const std::wstring& path);
    void PrepareDirs();

    void Run() override;

    std::wstring GetFileName(CroFile* file);
    void ExportCroFile(CroFile* file);

    ExportRecord ReadExportRecord(CroFile* file, CroEntry& entry);

    CroBuffer GetRecord(CroFile* file, CroEntry& entry, CroRecordTable* dat);
private:
    ICentaurusBank* m_pBank;
    std::wstring m_ExportPath;
};
#endif

#endif