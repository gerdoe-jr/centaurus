#include "crorecord.h"
#include "croexception.h"
#include "crofile.h"

/* CroRecord */

cronos_size CroRecord::RecordSize() const
{
    cronos_size size = 0;
    for (const auto& part : m_Data)
        size += part.m_PartSize;
    return size;
}

void CroRecord::AddPart(cronos_off off, cronos_size size)
{
    m_Data.emplace_back(off, size);
}

/* CroRecordMap */

CroBlock CroRecordMap::ReadBlock(cronos_off off, cronos_size size)
{
    CroBlock block = CroBlock(size == ABI()->Size(cronos_first_block_hdr));
    File()->Read(block, CRONOS_FILE_ID, CRONOS_DAT, off, size);
    
    return block;
}

CroRecord CroRecordMap::GetRecordMap(cronos_id id)
{
    CroRecord record;

    CroEntry entry = GetEntry(id);
    if (!entry.IsActive()) return record;
    
    bool hasBlock = entry.HasBlock();
    cronos_size blockSize = hasBlock 
        ? ABI()->Size(cronos_first_block_hdr) : 0;

    cronos_off recordNext = entry.EntryOffset();
    cronos_size recordSize = entry.EntrySize();

    if (hasBlock)
    {
        CroBlock block = ReadBlock(entry.EntryOffset(), blockSize);
        recordNext = block.BlockNext();
        recordSize = block.BlockSize();
    }
    
    cronos_off dataOff = entry.EntryOffset() + blockSize;
    cronos_size dataSize = std::min(recordSize,
        entry.EntrySize() - blockSize);

    record.AddPart(dataOff, dataSize);
    recordSize -= dataSize;

    blockSize = ABI()->Size(cronos_block_hdr);
    cronos_off defSize = File()->GetDefaultBlockSize();

    while (recordNext && recordSize > 0)
    {
        CroBlock block = ReadBlock(recordNext, blockSize);
        recordNext = block.BlockNext();
        
        dataOff = block.GetStartOffset() + blockSize;
        dataSize = std::min(recordSize, defSize - blockSize);

        record.AddPart(dataOff, dataSize);
        recordSize -= dataSize;
    }

    return record;
}

void CroRecordMap::Load()
{
    for (cronos_id id = IdStart(); id != IdEnd(); id++)
    {
        CroEntry entry = GetEntry(id);
        if (!entry.IsActive()) continue;

        m_Record[id] = GetRecordMap(id);
    }
}

CroBuffer CroRecordMap::LoadRecord(cronos_id id)
{
    auto file = File();
    auto& rec = m_Record[id];

    CroBuffer out;
    for (auto& [off, size] : rec.RecordParts())
    {
        CroData part = file->Read(id, CRONOS_DAT, off, size);
        out.Write(part.GetData(), part.GetSize());
    }

    if (file->IsEncrypted())
        file->Decrypt(out, id);

    if (file->IsCompressed())
    {
        throw CroException(file, "record is compressed");
    }

    return out;
}

bool CroRecordMap::HasRecord(cronos_id id) const
{
    if (!GetEntry(id).IsActive())
        return false;
    return m_Record.find(id) != m_Record.end();
}
