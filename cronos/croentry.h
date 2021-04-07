#ifndef __CROENTRY_H
#define __CROENTRY_H

#include "crotable.h"

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
    bool HasBlock() const;
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