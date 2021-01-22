#include "cronos.h"
#include "cronos02.h"
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

//TODO: CroFileBuffer::CroFileBuffer(CroFileBuffer&& other)

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

CroEntryTable::CroEntryTable(CroEntryTable& other)
    : CroFileBuffer(file, CROFILE_TAD)
{
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

/* CroFile */

bool CroFile::IsSupported(int major, int minor)
{
    static int majors[] = {1};
    static int minors[] = {2,3,4,11,13,14};

    if (std::find(std::begin(majors), std::end(majors), major)
            == std::end(majors))
    {
        SetError("unknown major " + std::to_string(major));
        return false;
    }

    if (std::find(std::begin(minors), std::end(minors), minor)
            == std::end(minors))
    {
        SetError("unknown minor " + std::to_string(minor));
        return false;
    }

    return true;
}

CroFile::CroFile(const std::wstring& path)
    : m_Path(path),m_iMajor(0),m_iVersion(0)
{
    SetError(CROFILE_OK);
    m_fDat = NULL;
    m_fTad = NULL;
    m_pCrypt = NULL;
    m_bEOB = false;
}

inline uint32_t _bswap32(uint32_t val)
{
    uint32_t res = val;
    
    asm volatile(
        "bswap %0"
        : "=r" (res)
        : "0" (res)
    );
    
    return res;
}

crofile_status CroFile::Open()
{
    FILE* fDat = _wfopen((m_Path + L".dat").c_str(), L"rb");
    FILE* fTad = _wfopen((m_Path + L".tad").c_str(), L"rb");
    if (!fDat || !fTad)
    {
        if (fDat) fclose(fDat);
        if (fTad) fclose(fTad);
        return SetError(CROFILE_FOPEN, std::string("CroFile::Open")
                + std::string(!fDat ? " !fDat" : "")
                + std::string(!fTad ? " !fTad" : "")
        );
    }

    m_fDat = fDat;
    m_fTad = fTad;

    fseek(m_fDat, 0L, SEEK_END);
    fseek(m_fTad, 0L, SEEK_END);
    m_DatSize = _ftelli64(m_fDat);
    m_TadSize = _ftelli64(m_fTad);

    fseek(m_fDat, 0L, SEEK_SET);
    fseek(m_fTad, 0L, SEEK_SET);


    char szHdr[20];
    fread(szHdr, sizeof(szHdr), 1, m_fDat);
    if (memcmp(szHdr, CRONOS_HEADER, 7))
    {
        Close();
        return SetError(CROFILE_HEADER, "invalid header");
    }

    char szMajor[4] = {0};
    char szVersion[4] = {0};
    memcpy(szMajor, szHdr+0x0A, 2);
    memcpy(szVersion, szHdr+0x0D, 2);
    m_iMajor = atoi(szMajor);
    m_iMinor = atoi(szVersion);
    if (!IsSupported(m_iMajor, m_iMinor))
    {
        Close();
        return UpdateError(CROFILE_VERSION, "unknown version");
    }

    if (m_iMinor < 11) m_iVersion = 3;
    else m_iVersion = 4;

    m_uFlags = *(uint16_t*)(szHdr+0x0F);
    m_uDefLength = *(uint16_t*)(szHdr+0x11);
    m_uTadRecordSize = m_iVersion >= 4 ? TAD_V4_BASE : TAD_V3_BASE;

    if (IsEncrypted())
    {
        uint32_t uSerial = 1;
        uint8_t szSecret[8] = {0};
        fseek(m_fDat, 0x13, SEEK_SET);
        fread(szSecret, 8, 1, m_fDat);
        memmove(szSecret+4, szSecret, 4);
        memcpy(szSecret, &uSerial, 4);

        m_pCrypt = (uint8_t*)malloc(0x200);
        if (m_iVersion == 3)
        {
            if (m_iMinor > 2)
            {
                fseek(m_fDat, 0xFC, SEEK_SET);
                fread(m_pCrypt, 0x200, 1, m_fDat);
            }
            else memcpy(m_pCrypt, cronos02_crypt_table, 0x200);
        }
        else if (m_iVersion >= 4)
        {
            fseek(m_fDat, 0xF8, SEEK_SET);
            fread(m_pCrypt, 0x200, 1, m_fDat);
        }

        FILE* fOut = fopen("table_enc.bin", "wb");
        fwrite(m_pCrypt, 0x200, 1, fOut);
        fclose(fOut);

        if (ferror(m_fDat) || feof(m_fDat))
        {
            free(m_pCrypt);
            return SetError("ferror/feof read m_pCrypt");
        }

        if (m_iMinor > 2)
        {
            blowfish_t* bf = new blowfish_t;
            blowfish_init(bf, szSecret, 8);
            blowfish_decrypt_buffer(bf, m_pCrypt, 0x200);
            delete bf;
        }

        fOut = fopen("table_dec.bin", "wb");
        fwrite(m_pCrypt, 0x200, 1, fOut);
        fclose(fOut);
    }

    return SetError(CROFILE_OK);
}

void CroFile::Close()
{
    if (m_fDat) fclose(m_fDat);
    if (m_fTad) fclose(m_fTad);
    m_fDat = NULL;
    m_fTad = NULL;

    if (m_pCrypt)
    {
        free(m_pCrypt);
        m_pCrypt = NULL;
    }
}

void CroFile::Reset()
{
    SetError();

    fseek(m_fDat, 0L, SEEK_SET);
    fseek(m_fTad, 0L, SEEK_SET);

    m_bEOB = false;
}

crofile_status CroFile::SetError(crofile_status st,
        const std::string& msg)
{
    m_Status = st;
    m_Error = msg;
    return m_Status;
}

crofile_status CroFile::SetError(const std::string& msg)
{
    m_Status = msg.empty() ? CROFILE_OK : CROFILE_ERROR;
    m_Error = msg;
    return m_Status;
}

crofile_status CroFile::UpdateError(crofile_status st,
        const std::string& msg)
{
    m_Status = st;
    if (!msg.empty())
        m_Error = m_Error + ", " + msg;
    return m_Status;
}

bool CroFile::IsFailed() const
{
    if (!m_Error.empty())
        return true;
    return false;
}

bool CroFile::IsEncrypted() const
{
    return m_uFlags & CRONOS_ENCRYPTION;
}

bool CroFile::IsCompressed() const
{
    return m_uFlags & CRONOS_COMPRESSION;
}

void CroFile::Decrypt(uint8_t* pBlock, unsigned size,
        uint32_t offset)
{
    if (!m_pCrypt)
    {
        SetError("Decrypt !m_pCrypt");
        return;
    }

    for (unsigned i = 0; i < size; i++)
    {
        pBlock[i] = m_pCrypt[0x100+pBlock[i]]
            - (uint8_t)(i + offset);
    }
}

FILE* CroFile::GetFile(crofile_type type) const
{
    switch (type)
    {
        case CROFILE_DAT: return m_fDat;
        case CROFILE_TAD: return m_fTad;
    }

    return NULL;
}

uint64_t CroFile::GetFileSize(crofile_type type) const
{
    switch (type)
    {
        case CROFILE_DAT: return m_DatSize;
        case CROFILE_TAD: return m_TadSize;
    }

    return 0;
}

uint64_t CroFile::ReadBuffer(CroBuffer* out, uint64_t offset,
        crofile_type type)
{
    FILE* fFile = GetFile(type);

    uint32_t size = out->GetSize();
    if (size > GetFileSize(type) - offset)
    {
        SetError("size > GetFileSize(type) - offset");
        return 0;
    }

    _fseeki64(fFile, offset, SEEK_SET);
    uint64_t read = fread(out->GetData(), size, 1, fFile);
    if (read < size)
        SetError("read < size");
    if (ferror(fFile))
        UpdateError(CROFILE_FREAD, "ferror");
    return read;
}

unsigned CroFile::EstimateEntryCount() const
{
    return (m_TadSize - m_uTadRecordSize) / m_uTadRecordSize;
}

bool CroFile::IsEndOfEntries() const
{
    return m_bEOB || feof(m_fTad);
}

#define CROFILE_TABLE_SIZE (uint64_t)64*1024*1024

uint32_t CroFile::GetOptimalEntryCount() const
{
    uint64_t pos_cur = _ftelli64(m_fTad);
    _fseeki64(m_fTad, 0L, SEEK_END);
    uint64_t pos_end = _ftelli64(m_fTad);
    _fseeki64(m_fTad, pos_cur, SEEK_SET);

    uint64_t uTadSize = pos_end - pos_cur;
    if (m_iVersion >= 4)
        return std::min(CROFILE_TABLE_SIZE, uTadSize) / TAD_V4_SIZE;
    else return std::min(CROFILE_TABLE_SIZE, uTadSize) / TAD_V3_SIZE;
}

CroEntryTable CroFile::LoadEntryTable(record_id idx, unsigned burst)
{
    if (idx == 0) m_bEOB = false;
    if (burst == 0) return CroEntryTable(*this, 0, 0);
    if (IsEndOfEntries()) return CroEntryTable(*this, 0, 0);
    if (burst > EstimateEntryCount())
        burst = EstimateEntryCount();

    CroEntryTable entries = CroEntryTable(*this, idx, burst);
    entries.Read();

    return entries;
}

/*BlockTable CroFile::LoadBlockTable(RecordTable& record, record_id i)
{
    BlockTable block(record);
    record_off start, end;
    if (record.BlockTableRange(i, &start, &end, CROFILE_TABLE_SIZE))
    {
        block.SetRange(start, end);
        _fseeki64(m_fDat, block.GetBlockTableOffset(), SEEK_SET);
        fread(block.GetBlockTableData(),
                block.GetBlockTableSize(), 1, m_fDat);
    }

    return block;
}*/

