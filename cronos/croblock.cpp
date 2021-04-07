#include "croblock.h"
#include "croentry.h"
#include "croexception.h"
#include "crofile.h"

CroBlockTable::CroBlockTable(CroEntryTable& tad,
    cronos_id id, cronos_idx count)
    : m_TAD(tad)
{
    InitEntity(tad.File(), id);
    m_uEntryCount = count;
}

CroBlock CroBlockTable::FirstBlock(cronos_id id) const
{
    CroBlock block(true);
    cronos_rel off = IdEntryOffset(id);

    block.SetOffset(off, GetFileType());
    block.InitEntity(File(), id);
    block.InitBuffer((uint8_t*)Data(off),
        ABI()->Size(cronos_first_block_hdr), false);
    return block;
}

bool CroBlockTable::NextBlock(CroBlock& block) const
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

cronos_rel CroBlockTable::IdEntryOffset(cronos_id id) const
{
    CroEntry entry = m_TAD.GetEntry(id);
    if (!entry.IsActive())
        throw CroException(File(), "record table inactive entry");
    return DataOffset(entry.EntryOffset());
}

unsigned CroBlockTable::GetEntrySize(cronos_id id) const
{
    if (id == INVALID_CRONOS_ID)
        return File()->GetDefaultBlockSize();

    CroBlock block = FirstBlock(id);
    return block.BlockSize();
}

unsigned CroBlockTable::GetEntryCount() const
{
    return CroTable::GetEntryCount();
}

void CroBlockTable::SetEntryCount(cronos_idx count)
{
    m_uEntryCount = count;
}
