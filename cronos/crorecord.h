#ifndef __CRORECORD_H
#define __CRORECORD_H

#include "croentry.h"
#include "croblock.h"
#include <vector>
#include <map>

class CroRecord
{
public:
    struct crorecord_part {
        cronos_off m_PartOff;
        cronos_size m_PartSize;
    };

    cronos_size RecordSize() const;
    void AddPart(cronos_off off, cronos_size size);
    inline const auto& RecordParts() const { return m_Data; }
private:
    std::vector<crorecord_part> m_Data;
};

class CroRecordMap : public CroEntryTable
{
public:
    CroBlock ReadBlock(cronos_off off, cronos_size size);
    CroRecord GetRecordMap(cronos_id id);
    inline auto& PartMap() { return m_Record; }
    inline const auto& PartMap() const { return m_Record; }

    void Load();
    CroBuffer LoadRecord(cronos_id id);
    bool HasRecord(cronos_id id) const;
private:
    std::map<cronos_id, CroRecord> m_Record;
};

#endif