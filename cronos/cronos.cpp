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
    if (IsEmpty()) m_pData = (uint8_t*)malloc(m_uSize + size);
    else m_pData = (uint8_t*)realloc(m_pData, m_uSize + size);
    m_uSize += size;
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

/* RecordTable */

RecordTable::RecordTable(record_id index, int version,
        unsigned count)
    : CroEntry(index, version)
{
    m_uCount = count;
    if (m_uCount)
        Alloc(m_uCount * GetRecordSize());
}

unsigned RecordTable::GetRecordSize() const
{
    return Is4A() ? TAD_V4_SIZE : TAD_V3_SIZE;
}

void RecordTable::SetRecordCount(unsigned count)
{
    m_uCount = count;
}

unsigned RecordTable::GetRecordCount() const
{
    return m_uCount;
}

RecordEntry RecordTable::GetRecordEntry(record_id i)
{
    return RecordEntry(Id() + i, Version(),
            GetTableData() + (i * GetRecordSize()));
}

bool RecordTable::FirstActiveRecord(record_off i, RecordEntry& entry)
{
    for (; i < GetRecordCount(); i++)
    {
        entry = GetRecordEntry(i);
        if (entry.IsActive())
            return true;
    }

    return false;
}

uint32_t RecordTable::BlockRecordCount(record_off i, unsigned sizeLimit)
{
    RecordEntry start;
    if (!FirstActiveRecord(i, start))
        return 0;

    RecordEntry end;
    record_off off;
    printf("records %u\n", GetRecordCount());
    /*for (off = start.Id() - Id(); off <= GetRecordCount(); off++)
    {
        end = GetRecordEntry(off);
        if (!end.IsActive())
            continue;

        unsigned size = start.GetSize() + (unsigned)
            (end.GetOffset() - start.GetOffset());
        if (size >= sizeLimit)
            break;
    }*/

    printf("[%u,\t%u]\t%016llx\t%016llx\n", i, off,
            start.GetOffset(), end.GetOffset());
    return off - i;
}

/* BlockTable */

BlockTable::BlockTable(RecordTable& record, record_id i,
        unsigned count)
    : m_Record(record)
{
    m_Id = record.Id() + i;
    m_iVersion = record.Version();
    m_uCount = count;

    RecordEntry start = m_Record.GetRecordEntry(i);
    RecordEntry end = m_Record.GetRecordEntry(i
            + GetBlockTableCount());
    m_uTableOffset = start.GetOffset();
    m_uTableSize = end.GetOffset() - m_uTableOffset;

    Alloc(m_uTableSize);
}

uint64_t BlockTable::GetBlockTableOffset() const
{
    return m_uTableOffset;
}

uint64_t BlockTable::GetBlockTableSize() const
{
    return m_uTableSize;
}

/* CroBlock */

CroBlock::CroBlock(uint8_t* data, uint32_t size,
            int version, bool first)
    : CroBuffer(data, size)
{
    m_iVersion = version;
    m_bFirst = first;
}

uint64_t CroBlock::GetNextBlock() const
{
    return *(uint64_t*)GetData();
}

uint32_t CroBlock::GetBlockSize() const
{
    return *(uint32_t*)(GetData()+0x08);
}

uint8_t* CroBlock::GetBlockData() const
{
    return GetData()+0x0C;
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

RecordTable CroFile::LoadRecordTable(record_id idx, unsigned burst)
{
    if (idx == 0) m_bEOB = false;
    if (burst == 0) return RecordTable(0, 0, 0);
    if (IsEndOfEntries()) return RecordTable(0, 0, 0);
    if (burst > EstimateEntryCount())
        burst = EstimateEntryCount();
    RecordTable table = RecordTable(idx, GetVersion(), burst);

    uint64_t base = m_iVersion >= 4 ? TAD_V4_BASE : TAD_V3_BASE;
    uint64_t off = base + (idx-1)*table.GetRecordSize();
    _fseeki64(m_fTad, off, SEEK_SET);
    unsigned uCount = fread(table.GetTableData(), table.GetRecordSize(),
            table.GetRecordCount(), m_fTad);
    if (ferror(m_fTad))
        SetError("LoadRecordTable ferror");

    return table;
}

BlockTable CroFile::LoadBlockTable(RecordTable& record, record_id i)
{
    uint32_t count = record.BlockRecordCount(i, CROFILE_TABLE_SIZE);
    BlockTable block = BlockTable(record, i, count);

    _fseeki64(m_fDat, block.GetBlockTableOffset(), SEEK_SET);
    fread(block.GetBlockTableData(), block.GetBlockTableSize(), 1, m_fDat);

    return block;
}

