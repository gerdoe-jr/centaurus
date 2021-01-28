#ifndef __CROTABLE_H
#define __CROTABLE_H

#include "crotype.h"
#include "crodata.h"

/* TAD */

class CroEntry : public CroData
{
public:
    CroEntry(const CroData& table, cronos_id id,
            cronos_off rel, cronos_size size);

    cronos_off EntryOffset() const;
    cronos_size EntrySize() const;
    uint32_t EntryFlags() const;

    bool IsActive() const;
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
