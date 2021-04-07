#include "croentry.h"
#include "cronos_format.h"

/* CroEntry */

bool CroEntry::IsActive() const
{
    if (Is3())
    {
        if (EntrySize() == TAD_V3_INVALID) return false;
        if (!EntryFlags() || EntryFlags() == TAD_V3_DELETED)
            return false;
    }
    else if (Is4A())
    {
        if (EntrySize() == TAD_V4_INVALID) return false;
        if (Get<uint64_t>(cronos_tad_rz) & TAD_V4_RZ_DELETED)
            return false;
    }

    return EntryOffset() && EntrySize();
}

bool CroEntry::HasBlock() const
{
    if (Is3())
    {
        if (Get<uint32_t>(cronos_tad_rz) & TAD_V3_RZ_NOBLOCK)
            return false;
    }

    return true;
}

/* CroEntryTable */

cronos_rel CroEntryTable::IdEntryOffset(cronos_id id) const
{
    cronos_idx idx = id - IdStart();
    return (cronos_off)idx * GetEntrySize();
}

unsigned CroEntryTable::GetEntrySize(cronos_id id) const
{
    return ABI()->Size(cronos_tad_entry);
}

CroEntry CroEntryTable::GetEntry(cronos_id id) const
{
    CroEntry entry;

    CroTable::GetEntry(id, entry);
    return entry;
}

unsigned CroEntryTable::GetEntryCount() const
{
    return ABI() ? GetSize() / GetEntrySize() : m_uEntryCount;
}

bool CroEntryTable::FirstActiveEntry(cronos_id id, CroEntry& entry)
{
    for (; id != IdEnd(); id++)
    {
        entry = GetEntry(id);
        if (entry.IsActive()) return true;
    }

    return false;
}
