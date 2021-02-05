#include "crotable.h"
#include "croexception.h"
#include "cronos_format.h"
#include <algorithm>

/* CroTable */

template<typename T>
CroTable<T>::CroTable(CroFile* file, cronos_id id, cronos_size limit)
{
    InitData(file, id, AssocFileType(), TableOffset(),
        std::min(GetEntryCount() * GetEntrySize(), limit));
}

template<typename T>
cronos_id CroTable<T>::IdStart() const
{
    return Id();
}

template<typename T>
cronos_id CroTable<T>::IdEnd() const
{
    return Id() + GetEntryCount();
}

template<typename T>
cronos_rel CroTable<T>::IdEntryOffset(cronos_id id) const
{
    if (!IsValidEntryId(id))
        throw CroException(File(), "invalid entry id");

    
}

/* CroEntry */

CroEntry::CroEntry(const CroData& table, cronos_id id,
        cronos_off rel, cronos_size size)
    : CroData(table, id, rel, size)
{
}

cronos_off CroEntry::EntryOffset() const
{
    return Is4A()
        ? TAD_V4_OFFSET(Get<uint64_t>(0x00))
        : TAD_V3_OFFSET(Get<uint32_t>(0x00));
}

cronos_size CroEntry::EntrySize() const
{
    return Is4A()
        ? TAD_V4_FSIZE(Get<uint32_t>(0x08))
        : TAD_V3_FSIZE(Get<uint32_t>(0x04));
}

cronos_flags CroEntry::EntryFlags() const
{
    return Is4A() ? Get<uint32_t>(0x0C) : Get<uint32_t>(0x08);
}

bool CroEntry::IsActive() const
{
    if (Is3())
    {
        if (EntryFlags() == 0 || EntryFlags() == 0xFFFFFFFF)
            return false;
    }
    else if (Is4A())
    {
        if (TAD_V4_RZ(Get<uint64_t>(0x00)) & TAD_V4_RZ_DELETED)
            return false;
    }

    return EntryOffset() && EntrySize();
}

/* CroEntryTable */

unsigned CroEntryTable::GetEntrySize() const
{
    return Is4A() ? TAD_V4_SIZE : TAD_V3_SIZE;
}

cronos_off CroEntryTable::IdEntryOffset(cronos_id id) const
{
    cronos_off base = Is4A() ? TAD_V4_BASE : TAD_V3_BASE;
    return base + (id-1)*GetEntrySize();
}

cronos_id CroEntryTable::IdStart() const
{
    return Id();
}

cronos_id CroEntryTable::IdEnd() const
{
    return IdStart() + GetEntryCount();
}

CroEntry CroEntryTable::GetEntry(cronos_id id) const
{
    cronos_idx idx = id - IdStart();
    unsigned size = GetEntrySize();
    return CroEntry(*this, id, idx*size, size);
}

unsigned CroEntryTable::GetEntryCount() const
{
    return GetSize() / GetEntrySize();
}
