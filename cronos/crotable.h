#ifndef __CROTABLE_H
#define __CROTABLE_H

#include "crotype.h"
#include "crodata.h"

/* CroTable */

template<typename T>
class CroTable : public CroData
{
public:
    CroTable(CroFile* file, cronos_id id, cronos_size limit);

    cronos_id IdStart() const;
    cronos_id IdEnd() const;

    virtual cronos_filetype AssocFileType() const = 0;

    virtual cronos_rel IdEntryOffset(cronos_id id) const;
    virtual unsigned GetEntrySize(cronos_id id
        = INVALID_CRONOS_ID) const;
    virtual T GetEntry(cronos_id id) const;
    virtual unsigned GetEntryCount() const;
protected:
    using Entry = T;
    using FileType = F;
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
    unsigned GetEntrySize(cronos_id id = INVALID_CRONOS_ID) const override;
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
