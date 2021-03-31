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

/* CroRecordTable */

CroRecordTable::CroRecordTable(CroEntryTable& tad,
    cronos_id id, cronos_idx count)
    : m_TAD(tad)
{
    InitEntity(tad.File(), id);
    m_uEntryCount = count;
}

CroBlock CroRecordTable::FirstBlock(cronos_id id) const
{
    CroBlock block(true);
    cronos_rel off = IdEntryOffset(id);

    block.SetOffset(off, GetFileType());
    block.InitEntity(File(), id);
    block.InitBuffer((uint8_t*)Data(off),
        ABI()->Size(cronos_first_block_hdr), false);
    return block;
}

bool CroRecordTable::NextBlock(CroBlock& block) const
{
    if (!block.BlockNext()) return false;
    if (!IsValidOffset(block.BlockNext()))
        return false;
    cronos_rel next = DataOffset(block.BlockNext());

    block.SetOffset(next, GetFileType());
    block.InitEntity(File(), Id());
    block.InitBuffer((uint8_t*)Data(next),
        ABI()->Size(cronos_block_hdr), false);
    return true;
}

cronos_rel CroRecordTable::IdEntryOffset(cronos_id id) const
{
    CroEntry entry = m_TAD.GetEntry(id);
    if (!entry.IsActive())
        throw CroException(File(), "record table inactive entry");
    return DataOffset(entry.EntryOffset());
}

unsigned CroRecordTable::GetEntrySize(cronos_id id) const
{
    if (id == INVALID_CRONOS_ID)
        return File()->GetDefaultBlockSize();
    
    CroBlock block = FirstBlock(id);
    return block.BlockSize();
}

unsigned CroRecordTable::GetEntryCount() const
{
    return CroTable::GetEntryCount();
}

void CroRecordTable::SetEntryCount(cronos_idx count)
{
    m_uEntryCount = count;
}
