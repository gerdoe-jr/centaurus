#include "crotable.h"
#include "croexception.h"
#include "cronos_abi.h"
#include "cronos_format.h"
#include <algorithm>

/* CroTable */

CroTable::CroTable()
{
    m_uEntryCount = 0;
}

void CroTable::InitTable(CroFile* file, cronos_filetype ftype,
    cronos_id start, cronos_id end, cronos_size limit)
{
    InitEntity(file, start);
    InitData(file, start, ftype, GetStartOffset(), limit);

    m_uEntryCount = end - start;
}

void CroTable::InitTable(CroFile* file, cronos_filetype ftype,
    cronos_id start, cronos_size limit)
{
    InitEntity(file, start);
    InitData(file, start, ftype, GetStartOffset(), limit);

    m_uEntryCount = (unsigned)(limit / GetEntrySize());
}

void CroTable::Sync()
{
    m_uEntryCount = GetSize() / GetEntrySize();
}

cronos_id CroTable::IdStart() const
{
    return Id();
}

cronos_id CroTable::IdEnd() const
{
    return Id() + GetEntryCount();
}

cronos_off CroTable::TableOffset() const
{
    return FileOffset(IdEntryOffset(IdStart()));
}

cronos_size CroTable::TableSize() const
{
    return (cronos_size)GetEntryCount() * GetEntrySize();
}

bool CroTable::IsValidEntryId(cronos_id id) const
{
    return id >= IdStart() && id < IdEnd();
}

cronos_rel CroTable::IdEntryOffset(cronos_id id) const
{
    cronos_idx idx = id - IdStart();
    return (cronos_rel)idx * GetEntrySize();
}

void CroTable::GetEntry(cronos_id id, CroData& out) const
{
    out = CroData(*this, id, IdEntryOffset(id), GetEntrySize(id));
}

unsigned CroTable::GetEntrySize(cronos_id id) const
{
    throw CroException(File(), "CroTable::GetEntrySize");
}

unsigned CroTable::GetEntryCount() const
{
    return m_uEntryCount;
}

/* CroEntry */

bool CroEntry::IsActive() const
{
    if (Is3())
    {
        if (!EntryFlags() || EntryFlags() == 0xFFFFFFFF)
            return false;
    }
    else if (Is4A())
    {
        if (Get<uint64_t>(cronos_tad_rz) & TAD_V4_RZ_DELETED)
            return false;
    }

    return EntryOffset() && EntrySize();
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
    return GetSize() / GetEntrySize();
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
