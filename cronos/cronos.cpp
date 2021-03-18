#include "cronos.h"
#include "cronos02.h"
#include "cronos_format.h"
#include <stdexcept>
#include <algorithm>
#include <string.h>
#include <stdio.h>

extern "C"
{
    #include "blowfish.h"
}

/* CroEntry */

CroEntry::CroEntry()
{
    m_Id = 0;
    m_iVersion = -1;
}

CroEntry::CroEntry(record_id id, int version)
    : m_Id(id), m_iVersion(version)
{
}

record_id CroEntry::Id() const
{
    return m_Id;
}

int CroEntry::Version() const
{
    return m_iVersion;
}

bool CroEntry::IsVersion(int version) const
{
    return version == Version();
}

/* CroBuffer */

CroBuffer::CroBuffer()
{
    m_pData = NULL;
    m_uSize = 0;
}

CroBuffer::CroBuffer(uint8_t* data, uint32_t size)
    : m_pData(data), m_uSize(size)
{
}

CroBuffer::CroBuffer(CroBuffer&& src)
{
    m_pData = src.Dereference(m_uSize);
}

CroBuffer::~CroBuffer()
{
    if (!IsEmpty()) free(m_pData);
}

bool CroBuffer::IsEmpty() const
{
    return m_uSize == 0;
}

uint8_t* CroBuffer::GetData() const
{
    if (IsEmpty()) throw std::runtime_error("CroBuffer is empty");
    return m_pData;
}

uint8_t* CroBuffer::Dereference(uint32_t& size)
{
    uint8_t* data = m_pData;
    size = m_uSize;

    m_pData = NULL;
    m_uSize = 0;

    return data;
}

void CroBuffer::Alloc(uint32_t size)
{
    if (size == 0) return;

    if (IsEmpty()) m_pData = (uint8_t*)malloc(m_uSize + size);
    else m_pData = (uint8_t*)realloc(m_pData, size);
    m_uSize = size;
}

void CroBuffer::Write(uint8_t* data, uint32_t size)
{
    uint32_t uNewSize = m_uSize + size;
    if (IsEmpty()) m_pData = (uint8_t*)malloc(uNewSize);
    else m_pData = (uint8_t*)realloc(m_pData, uNewSize);

    if (!m_pData) throw std::runtime_error("CroBuffer !m_pData");
    memcpy(m_pData+m_uSize, data, size);
    m_uSize += size;
}

/* CroFileBuffer */

CroFileBuffer::CroFileBuffer(CroFile& file, crofile_type type)
    : m_File(file), m_Type(type)
{
    m_Id = 0;
    m_iVersion = m_File.GetVersion();

    m_uFileOffset = 0;
    m_uBufferSize = 0;
}

CroFileBuffer::CroFileBuffer(CroFileBuffer&& other)
    : CroBuffer(other)
{
}

CroFileBuffer::~CroFileBuffer()
{
}

void CroFileBuffer::Assign(uint64_t startOff, uint64_t size,
        record_id idx)
{
    m_uFileOffset = startOff;
    m_uBufferSize = size;

    if (idx) m_Id = idx;

    Alloc(GetBufferSize());
}

void CroFileBuffer::Read()
{
    uint64_t read = m_File.ReadBuffer(this, m_uFileOffset, m_Type);
    if (read < GetBufferSize())
        throw std::runtime_error("read < GetBufferSize()");
}

uint64_t CroFileBuffer::GetStartOffset() const
{
    return m_uFileOffset;
}

uint64_t CroFileBuffer::GetEndOffset() const
{
    return GetStartOffset() + GetBufferSize();
}

crofile_offset_type CroFileBuffer::CheckOffset(uint64_t offset) const
{
    if (offset >= GetStartOffset() && offset < GetEndOffset())
        return CROFILE_OFFSET_OK;
    else if (offset < GetStartOffset() || offset > GetEndOffset())
        return CROFILE_OFFSET_OUTSIDE;
    else return CROFILE_OFFSET_INVALID;
}

uint8_t* CroFileBuffer::DataPointer(uint64_t offset) const
{
    if (CheckOffset(offset) != CROFILE_OFFSET_OK)
        throw std::runtime_error("CheckOffse !CROFILE_OFFSET_OK");
    return GetFileData() + offset;
}

/* RecordEntry */

RecordEntry::RecordEntry()
{
    m_pEntry = NULL;
}

RecordEntry::RecordEntry(record_id id, int version,
        uint8_t* entry)
    : CroEntry(id, version)
{
    m_pEntry = entry;
}

uint64_t RecordEntry::GetOffset() const
{
    return Is4A()
        ? TAD_V4_OFFSET(*(uint64_t*)m_pEntry)
        : *(uint32_t*)m_pEntry;
}

uint32_t RecordEntry::GetSize() const
{
    uint32_t size = Is4A()
        ? *(uint32_t*)(m_pEntry+0x08)
        : TAD_V3_FSIZE(*(uint32_t*)(m_pEntry+0x04));
    return size != 0xFFFFFFFF ? size : 0;
}

uint32_t RecordEntry::GetFlags() const
{
     return Is4A()
        ? *(uint32_t*)(m_pEntry+0x0C)
        : *(uint32_t*)(m_pEntry+0x08);
}

bool RecordEntry::IsActive() const
{
    if (Is3())
    {
        if (GetFlags() == 0 || GetFlags() == 0xFFFFFFFF)
            return false;
    }
    else if (Is4A())
    {
        if (TAD_V4_RZ(*(uint64_t*)m_pEntry) & TAD_V4_RZ_DELETED)
            return false;
    }
    return GetOffset() && GetSize();
}

/* CroEntryTable */

CroEntryTable::CroEntryTable(CroFile& file, record_id idx,
        unsigned count)
    : CroFileBuffer(file, CROFILE_TAD)
{
    Assign(CalcOffset(idx), count * GetEntrySize(),
            idx);
}

unsigned CroEntryTable::GetEntrySize() const
{
    if (Is4A()) return TAD_V4_SIZE;
    else return TAD_V3_SIZE;
}

uint64_t CroEntryTable::CalcOffset(record_id idx) const
{
    return (Is4A() ? TAD_V4_BASE : TAD_V3_BASE)
        + GetEntrySize() * idx;
}

void CroEntryTable::SetEntryCount(unsigned count)
{
    Assign(CalcOffset(Id()), count * GetEntrySize(),
            Id());
}

unsigned CroEntryTable::GetEntryCount() const
{
    return GetBufferSize() / GetEntrySize();
}

RecordEntry CroEntryTable::GetRecordEntry(record_id i)
{
    return RecordEntry(Id() + i, Version(),
            DataPointer(i * GetEntrySize()));
}

bool CroEntryTable::FirstActiveRecord(record_off i, RecordEntry& entry)
{
    for (; i < GetEntryCount(); i++)
    {
        entry = GetRecordEntry(i);
        if (entry.IsActive())
            return true;
    }

    return false;
}

bool CroEntryTable::RecordTableRange(record_off i, record_off* pStart,
        record_off* pEnd, unsigned sizeLimit)
{
    RecordEntry start;
    if (!FirstActiveRecord(i, start))
        return false;

    uint64_t prevOffset = start.GetOffset();
    unsigned size = start.GetSize();
    RecordEntry end;
    record_off off = i;

    if (Id()+i > 1)
    {
        RecordEntry prev = GetRecordEntry(i-1);
        if (prev.IsActive())
        {
            if (start.GetOffset() < prev.GetOffset())
            {
                *pEnd = *pStart = i;
                return true;
            }
        }
    }

    while (size < sizeLimit)
    {
        if (off == GetEntryCount())
        {
            off--;
            break;
        }

        end = GetRecordEntry(++off);
        if (!end.IsActive())
            continue;
        if (end.GetOffset() < prevOffset)
        {
            off--;
            break;
            }
        prevOffset = end.GetOffset();
        size = (unsigned)(end.GetOffset() - start.GetOffset());
    }

    *pStart = start.Id() - Id();
    *pEnd = off;
    return true;
}

/* CroRecord */

CroRecord::CroRecord(record_id i, CroEntryTable& entries,
        CroRecordTable& records)
    : m_Entries(entries), m_Records(records)
{
    m_Id = m_Records.Id() + i;
}

record_off CroRecord::EntryOffset()
{
    return Id() - m_Entries.Id();
}

record_off CroRecord::RecordOffset()
{
    return Id() - m_Records.Id();
}

uint64_t CroRecord::NextBlockOffset(uint8_t* block)
{
    if (Is4A()) return TAD_V4_OFFSET(*(uint64_t*)block);
    else return TAD_V3_OFFSET(*(uint32_t*)block);
}

uint32_t CroRecord::BlockSize(uint8_t* block)
{
    if (Is4A()) return TAD_V4_FSIZE(*(uint32_t*)(block+0x08));
    else return TAD_V3_FSIZE(*(uint32_t*)(block+0x04));
}

uint8_t* CroRecord::BlockData(uint8_t* block, bool first)
{
    if (Is4A()) return block + (first ? 0x0C : 0x08);
    else return block + (first ? 0x08 : 0x04);
}

uint8_t* CroRecord::FirstBlock(uint32_t* pSize, uint64_t* pNext)
{
    RecordEntry entry = m_Entries.GetRecordEntry(EntryOffset());
    if (!entry.IsActive())
        return NULL;

    *pSize = entry.GetSize();
    return m_Records.DataPointer(entry.GetOffset());
}

/* CroRecordTable */

CroRecordTable::CroRecordTable(CroFile& file, CroEntryTable& entries)
    : CroFileBuffer(file, CROFILE_DAT),
    m_Entries(entries)
{
    m_Start = m_End = 0;
}

void CroRecordTable::SetRange(record_off start, record_off end)
{
    uint64_t size;
    uint64_t startOffset, endOffset;

    m_Start = start;
    m_End = end;

    RecordEntry entry = m_Entries.GetRecordEntry(m_Start);
    startOffset = entry.GetOffset();

    if (m_Start == m_End)
    {
        endOffset = startOffset + std::min(
                entry.GetSize(), (uint32_t)4096);
    }
    else
    {
        entry = m_Entries.GetRecordEntry(m_End);
        endOffset = entry.GetOffset();
    }

    size = endOffset - startOffset + entry.GetSize();
    Assign(Id() + start, startOffset, size);
}

void CroRecordTable::GetRange(record_off* pStart, record_off* pEnd) const
{
    *pStart = m_Start;
    *pEnd = m_End;
}

unsigned CroRecordTable::GetRecordCount() const
{
    return m_End - m_Start + 1;
}
