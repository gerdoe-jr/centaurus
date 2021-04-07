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

    virtual cronos_rel IdEntryOffset(cronos_id id) const;

    virtual unsigned GetEntrySize(cronos_id id
        = INVALID_CRONOS_ID) const;
    virtual unsigned GetEntryCount() const;
protected:
    void GetEntry(cronos_id id, CroData& out) const;

    cronos_idx m_uEntryCount;
};

#endif
