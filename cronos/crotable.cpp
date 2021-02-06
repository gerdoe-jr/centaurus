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

CroTable::CroTable(CroFile* file, cronos_filetype ftype,
    cronos_id id, cronos_size limit)
{
    m_uEntryCount = limit / GetEntrySize(id);
    InitData(file, id, ftype,
        FileOffset(IdEntryOffset(id)),
        std::min(((cronos_size)GetEntryCount() * GetEntrySize()), limit)
    );
}

CroTable::CroTable(CroFile* file, cronos_filetype ftype,
    cronos_id start, cronos_id end,
    cronos_size size)
{
    m_uEntryCount = end - start;
    InitData(file, start, ftype, FileOffset(IdEntryOffset(start)),
        size);
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
    unsigned count = GetEntryCount();
    return count
        ? (cronos_size)count * GetEntrySize()
        : GetSize();
}

bool CroTable::IsValidEntryId(cronos_id id) const
{
    return id >= IdStart() && id < IdEnd();
}

cronos_rel CroTable::IdEntryOffset(cronos_id id) const
{
    if (!IsValidEntryId(id))
        throw CroException(File(), "invalid entry id");

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
        if (ABI()->GetValue(*this, cronos_tad_rz) & TAD_V4_RZ_DELETED)
            return false;
    }

    return EntryOffset() && EntrySize();
}

/* CroEntryTable */

cronos_off CroEntryTable::IdEntryOffset(cronos_id id) const
{
    return ABI()->Offset(cronos_tad_base)
        + (cronos_off)id * GetEntrySize();
}

unsigned CroEntryTable::GetEntrySize(cronos_id id) const
{
    return ABI()->GetFormatSize(cronos_tad_entry);
}

CroEntry CroEntryTable::GetEntry(cronos_id id) const
{
    CroEntry entry;

    CroTable::GetEntry(id, entry);
    return entry;
}

unsigned CroEntryTable::GetEntryCount() const
{
    return TableSize() / GetEntrySize();
}
