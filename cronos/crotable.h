#ifndef __CROTABLE_H
#define __CROTABLE_H

#include "crotype.h"
#include "crodata.h"

/* CroTable */

template<typename T>
class CroTable : public CroData
{
public:
    CroTable();
    CroTable(CroFile* file, cronos_filetype ftype,
        cronos_id id, cronos_size limit);
    CroTable(CroFile* file, cronos_filetype ftype,
        cronos_id start, cronos_id end,
        cronos_size size);

    cronos_id IdStart() const;
    cronos_id IdEnd() const;

    cronos_off TableOffset() const;
    cronos_size TableSize() const;

    bool IsValidEntryId(cronos_id id) const;

    virtual cronos_rel IdEntryOffset(cronos_id id) const = 0;
    virtual unsigned GetEntrySize(cronos_id id
        = INVALID_CRONOS_ID) const = 0;
    
    virtual void GetEntry(cronos_id id, CroData& out) const;
    virtual unsigned GetEntryCount() const;
private:
    unsigned m_uEntryCount;
};

/* TAD */

class CroEntry : public CroData
{
public:
    CroEntry(const CroData& table, cronos_id id,
            cronos_off rel, cronos_size size);

    cronos_off EntryOffset() const;
    cronos_size EntrySize() const;
    cronos_flags EntryFlags() const;

    bool IsActive() const;
};

class CroEntryTable : public CroTable<CroEntry>
{
public:
    unsigned GetEntrySize(cronos_id id) const override;
    CroEntry GetEntry(cronos_id id) const override;
    unsigned GetEntryCount() const override;

    bool FirstActiveEntry(cronos_id id, CroEntry& entry);
};

class CroEntryTable : public CroData
{
public:
    unsigned GetEntrySize() const;
    cronos_off IdEntryOffset(cronos_id id) const;

    cronos_id IdStart() const;
    cronos_id IdEnd() const;
    CroEntry GetEntry(cronos_id id) const;
    unsigned GetEntryCount() const;

    bool FirstActiveEntry(cronos_id id, CroEntry& entry);
};

#endif
