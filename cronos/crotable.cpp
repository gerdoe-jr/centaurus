#include "crotable.h"
#include "croexception.h"
#include "crofile.h"
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
    return Id() + m_uEntryCount;
}

cronos_off CroTable::TableOffset() const
{
    return ABI() ? FileOffset(IdEntryOffset(IdStart()))
        : GetStartOffset();
}

cronos_size CroTable::TableSize() const
{
    return ABI() ? (cronos_size)GetEntryCount() * GetEntrySize()
        : GetSize();
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
