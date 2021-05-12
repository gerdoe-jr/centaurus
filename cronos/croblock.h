#ifndef __CROBLOCK_H
#define __CROBLOCK_H

#include "crotable.h"

class CroBlock : public CroData
{
public:
    CroBlock(bool first = true) : m_bFirst(first)
    {
    }

    inline cronos_off BlockNext() const
    {
        return Get<cronos_off>(cronos_block_next);
    }

    inline cronos_size BlockSize() const
    {
        return Get<cronos_size>(cronos_first_block_size);
    }

    inline bool HasNext() const
    {
        return BlockNext() != 0;
    }

    inline cronos_off PartOffset() const
    {
        return GetStartOffset() + ABI()->Size(m_bFirst
            ? cronos_first_block_hdr : cronos_block_hdr);
    }
private:
    bool m_bFirst;
};

class CroEntryTable;

class CroBlockTable : public CroTable
{
public:
    CroBlockTable(CroEntryTable& tad, cronos_id id, cronos_idx count);

    CroBlock FirstBlock(cronos_id id) const;
    bool NextBlock(CroBlock& block) const;

    cronos_rel IdEntryOffset(cronos_id id) const override;
    unsigned GetEntrySize(cronos_id id = INVALID_CRONOS_ID) const override;
    unsigned GetEntryCount() const override;

    void SetEntryCount(cronos_idx count);
private:
    CroEntryTable& m_TAD;
};

#endif