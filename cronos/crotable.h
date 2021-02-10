#ifndef __CROTABLE_H
#define __CROTABLE_H

#include "crotype.h"
#include "crodata.h"
#include "cronos_abi.h"

/* CroTable */

class CroTable : public CroData
{
public:
    CroTable();

    void InitTable(CroFile* file, cronos_filetype ftype,
        cronos_id start, cronos_id end, cronos_size limit);
    void InitTable(CroFile* file, cronos_filetype ftype,
        cronos_id start, cronos_size limit);
    void Sync();

    cronos_id IdStart() const;
    cronos_id IdEnd() const;

    cronos_off TableOffset() const;
    cronos_size TableSize() const;

    bool IsValidEntryId(cronos_id id) const;

    virtual cronos_rel IdEntryOffset(cronos_id id) const = 0;

    virtual unsigned GetEntrySize(cronos_id id
        = INVALID_CRONOS_ID) const;
    virtual unsigned GetEntryCount() const;
protected:
    void GetEntry(cronos_id id, CroData& out) const;
private:
    unsigned m_uEntryCount;
};

/* TAD */

class CroEntry : public CroData
{
public:
    inline cronos_off EntryOffset() const
    {
        return Get<cronos_off>(cronos_tad_offset);
    }

    inline cronos_size EntrySize() const
    {
        return Get<cronos_off>(cronos_tad_size);
    }

    inline cronos_flags EntryFlags() const
    {
        return Get<cronos_off>(cronos_tad_flags);
    }

    bool IsActive() const;
};

class CroEntryTable : public CroTable
{
public:
    cronos_rel IdEntryOffset(cronos_id id) const override;
    unsigned GetEntrySize(cronos_id id = INVALID_CRONOS_ID) const override;
    CroEntry GetEntry(cronos_id id) const;
    unsigned GetEntryCount() const override;

    bool FirstActiveEntry(cronos_id id, CroEntry& entry);
};

#endif
