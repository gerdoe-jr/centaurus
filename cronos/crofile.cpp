#include "crofile.h"
#include "cronos_abi.h"
#include "cronos_format.h"
#include "croexception.h"
#include <algorithm>
#include <string.h>

extern "C"
{
    #include "blowfish.h"
}

CroFile::CroFile(const std::wstring& path)
    : m_Path(path)
{
    m_Version = INVALID_CRONOS_VERSION;

    SetError(CROFILE_OK);
    m_fDat = NULL;
    m_fTad = NULL;
    m_bEOB = false;
}

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
    m_uDatSize = _ftelli64(m_fDat);
    m_uTadSize = _ftelli64(m_fTad);

    m_uSerial = 1;
    Reset();

    CroData hdr = ABI()->ReadData(this, cronos_hdr);

    const uint8_t* sig = hdr.Data(ABI()->Offset(cronos_hdr_sig));
    if (!memcmp(sig, CRONOS_HEADER, 7))
    {
        Close();
        return SetError(CROFILE_HEADER, "invalid header");
    }

    char szMajor[4] = {0};
    char szMinor[4] = {0};
    memcpy(szMajor, ABI()->GetPtr(hdr, cronos_hdr_major), 2);
    memcpy(szMinor, ABI()->GetPtr(hdr, cronos_hdr_minor), 2);

    cronos_abi_num abiVersion = cronos_abi_version(
        atoi(szMajor), atoi(szMinor));
    m_pABI = CronosABI::LoadABI(abiVersion);
    if (!ABI())
    {
        return SetError(CROFILE_VERSION, std::string("unknown ABI")
            + szMajor + "." + szMinor);
    }

    m_uFlags = ABI()->Get<uint16_t>(hdr, cronos_hdr_flags);
    m_uDefLength = ABI()->Get<uint16_t>(hdr, cronos_hdr_deflength);
    m_uTadRecordSize = ABI()->GetFormatSize(cronos_tad_entry);

    if (IsEncrypted())
    {
        CroData key;

        ABI()->GetData(hdr, cronos_hdr_secret, key);
        memmove(key.GetData() + 4, key.GetData(), 4);
        memcpy(key.GetData(), &m_uSerial, 4);

        ABI()->GetData(hdr, cronos_hdr_crypt, m_Crypt);
        if (!ABI()->Is3() && ABI()->Minor() > 2)
        {
            blowfish_t* bf = new blowfish_t;
            blowfish_init(bf, key.GetData(), key.GetSize());
            blowfish_decrypt_buffer(bf, m_Crypt.GetData(),
                m_Crypt.GetSize());
            delete bf;
        }
    }

    FILE* fOut = fopen("table_dec.bin", "wb");
    fwrite(m_Crypt.GetData(), m_Crypt.GetSize(), 1, fOut);
    fclose(fOut);

    return SetError(CROFILE_OK);
}

void CroFile::Close()
{
    if (m_fDat) fclose(m_fDat);
    if (m_fTad) fclose(m_fTad);
    m_fDat = NULL;
    m_fTad = NULL;
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
    if (m_Crypt.IsEmpty())
    {
        SetError("Decrypt !m_Crypt");
        return;
    }

    for (unsigned i = 0; i < size; i++)
    {
        pBlock[i] = m_Crypt.GetData()[0x100+pBlock[i]]
            - (uint8_t)(i + offset);
    }
}

unsigned CroFile::EstimateEntryCount() const
{
    return (m_uTadSize - m_uTadRecordSize) / m_uTadRecordSize;
}

bool CroFile::IsEndOfEntries() const
{
    return m_bEOB || feof(m_fTad);
}

#define CROFILE_TABLE_SIZE (cronos_size)64*1024*1024

uint32_t CroFile::GetOptimalEntryCount() const
{
    uint64_t pos_cur = _ftelli64(m_fTad);
    _fseeki64(m_fTad, 0L, SEEK_END);
    uint64_t pos_end = _ftelli64(m_fTad);
    _fseeki64(m_fTad, pos_cur, SEEK_SET);

    uint64_t uTadSize = pos_end - pos_cur;
    if (GetVersion() >= 4)
        return std::min(CROFILE_TABLE_SIZE, uTadSize) / TAD_V4_SIZE;
    else return std::min(CROFILE_TABLE_SIZE, uTadSize) / TAD_V3_SIZE;
}

bool CroFile::IsValidOffset(cronos_off off, cronos_filetype type) const
{
    if (off == INVALID_CRONOS_OFFSET)
        return false;

    switch (type)
    {
        case CRONOS_TAD: return off < m_uTadSize;
        case CRONOS_DAT: return off < m_uDatSize;
    }

    return false;
}

FILE* CroFile::FilePointer(cronos_filetype ftype) const
{
    return ftype == CRONOS_TAD ? m_fTad: m_fDat;
}

cronos_off CroFile::GetOffset(cronos_filetype ftype) const
{
    return (cronos_off)_ftelli64(FilePointer(ftype));
}

void CroFile::Seek(cronos_off off, cronos_filetype ftype)
{
    if (!IsValidOffset(off, ftype))
        throw CroException(this, "CroFile::Seek invalid offset");
    _fseeki64(FilePointer(ftype), off, SEEK_SET);
}

void CroFile::Read(CroData& data, uint32_t count, cronos_size size)
{
    FILE* fp = FilePointer(data.GetFileType());
    cronos_size totalsize = count*size;
    Seek(data.GetStartOffset(), data.GetFileType());

    uint32_t read = fread(data.GetData(), count, size,
            FilePointer(data.GetFileType()));
    if (read < count)
    {
        if (ferror(fp)) throw CroStdError(this);
        else if (feof(fp)) m_bEOB = true;
        data.Alloc(read*size);
    }
}

void CroFile::LoadTable(cronos_filetype ftype, cronos_id id,
    cronos_size limit, CroTable& table)
{
    table.InitData(this, id, ftype, table.IdEntryOffset(id), limit);
    Read(table, 1, table.TableSize());
}

CroEntryTable CroFile::LoadEntryTable(cronos_id id, unsigned burst)
{
    CroEntryTable table;

    LoadTable(CRONOS_TAD, id, burst * table.GetEntrySize(), table);
    return table;
}

/*CroEntryTable CroFile::LoadEntryTable(record_id idx, unsigned burst)
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

BlockTable CroFile::LoadBlockTable(RecordTable& record, record_id i)
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
