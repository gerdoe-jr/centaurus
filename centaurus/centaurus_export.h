#ifndef __CENTAURUS_EXPORT_H
#define __CENTAURUS_EXPORT_H

#ifdef CENTAURUS_INTERNAL
#include "centaurus.h"
#include "centaurus_task.h"
#include <string>

class CCentaurusExport : public CCentaurusTask
{
public:
    CCentaurusExport(ICentaurusBank* bank, const std::wstring& path);
    void PrepareDirs();

    void Run() override;

    std::wstring GetFileName(CroFile* file);
    void ExportCroFile(CroFile* file);
    CroBuffer GetRecord(CroFile* file, CroEntry& entry, CroRecordTable* dat);
private:
    ICentaurusBank* m_pBank;
    std::wstring m_ExportPath;
};
#endif

#endif