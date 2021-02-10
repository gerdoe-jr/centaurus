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

void CroFile::DumpABI(CronosABI* abi)
{
    cronos_abi_num abiVersion = abi->GetABIVersion();
    printf("Cronos %dx %s %s, ABI %02d.%02d\n", abi->GetVersion(),
        abi->IsLite() ? "Lite" : "Pro",
        abi->GetModel() == cronos_model_big ? "big model" : "small model",
        abiVersion.first, abiVersion.second
    );
    for (unsigned i = 0; i < cronos_last; i++)
    {
        cronos_value value = (cronos_value)i;
        printf("%016" FCroOff "\t%" FCroSize "\t%s\n",
            abi->Offset(value), abi->Size(value),
            abi->GetValueName((cronos_value)i)
        );
    }
    printf("\n");
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

    //Generic ABI
    m_pABI = CronosABI::GenericABI();
    CroData hdr = Read(INVALID_CRONOS_ID, 1, cronos_hdr);

    if (memcmp(hdr.Data(cronos_hdr_sig), CRONOS_HEADER, 7))
    {
        Close();
        return SetError(CROFILE_HEADER, "invalid header");
    }

    char szMajor[4] = {0};
    char szMinor[4] = {0};
    memcpy(szMajor, hdr.Data(cronos_hdr_major), 2);
    memcpy(szMinor, hdr.Data(cronos_hdr_minor), 2);

    cronos_abi_num abiVersion = cronos_abi_version(
        atoi(szMajor), atoi(szMinor));
    m_pABI = CronosABI::GetABI(abiVersion);
    if (!ABI())
    {
        return SetError(CROFILE_VERSION, std::string("unknown ABI ")
            + szMajor + "." + szMinor);
    }
    //ABI initialized
    DumpABI(m_pABI);

    m_uFlags = hdr.Get<uint16_t>(cronos_hdr_flags);
    m_uDefLength = hdr.Get<uint16_t>(cronos_hdr_deflength);
    m_uTadRecordSize = ABI()->Size(cronos_tad_entry);

    if (IsEncrypted())
    {
        CroData key = hdr.Value(cronos_hdr_secret);
        memmove(key.GetData() + 4, key.GetData(), 4);
        memcpy(key.GetData(), &m_uSerial, 4);

        CroData crypt = hdr.Value(cronos_hdr_crypt);
        if (ABI()->GetModel() != cronos_model_small)
        {
            blowfish_t* bf = new blowfish_t;
            blowfish_init(bf, key.GetData(), key.GetSize());
            blowfish_decrypt_buffer(bf, crypt.GetData(),
                crypt.GetSize());
            delete bf;
        }

        m_Crypt.Copy(crypt.GetData(), crypt.GetSize());
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

    Seek(0, CRONOS_TAD);
    Seek(0, CRONOS_DAT);

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
    return ftype == CRONOS_TAD ? m_fTad : m_fDat;
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
    if (data.GetFileType() == CRONOS_INVALID_FILETYPE)
        throw CroException(this, "Read invalid filetype");

    FILE* fp = FilePointer(data.GetFileType());
    cronos_size totalsize = count*size;
    Seek(data.GetStartOffset(), data.GetFileType());
    
    size_t read = fread(data.GetData(), size, count, fp);
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
    table.InitTable(this, ftype, id, limit);
    Read(table, table.GetEntryCount(), table.GetEntrySize());
    table.Sync();
}

cronos_idx CroFile::OptimalEntryCount()
{
    cronos_off offset = GetOffset(CRONOS_TAD);
    if (!offset)
        offset = ABI()->Offset(cronos_tad_entry);
    cronos_size remaining = std::min(CROFILE_TAD_TABLE_LIMIT,
        m_uTadSize - offset);
    return remaining / ABI()->Size(cronos_tad_entry);
}

CroEntryTable CroFile::LoadEntryTable(cronos_id id, unsigned count)
{
    CroEntryTable table;
    cronos_size entrySize = ABI()->Size(cronos_tad_entry);

    table.SetOffset(ABI()->Offset(cronos_tad_entry));
    LoadTable(CRONOS_TAD, id, count * entrySize, table);
    return table;
}

cronos_idx CroFile::OptimalRecordCount(CroEntryTable& tad, cronos_id start)
{
    CroEntry entry;

    tad.FirstActiveEntry(start, entry);
    cronos_size tableSize = entry.EntrySize();

    while (tableSize < CROFILE_DAT_TABLE_LIMIT || !entry.IsActive())
    {
        if (entry.Id() + 1 == tad.IdEnd())
            break;
        cronos_off prev = entry.EntryOffset();
        entry = tad.GetEntry(entry.Id() + 1);
        cronos_off next = entry.EntryOffset();

        if (next < prev)
            throw CroException(this, "entry offset out of sequence!");
        
        tableSize += next - prev;
    }

    return entry.Id() - start + 1;
}

void CroFile::RecordTableOffsets(CroEntryTable& tad, cronos_id id,
    cronos_idx count, cronos_off& start, cronos_off& end)
{
    CroEntry entryStart, entryEnd;
    if (!tad.FirstActiveEntry(id, entryStart))
        throw CroException(this, "no active entry");
    start = entryStart.EntryOffset();

    entryEnd = tad.GetEntry(id + count - 1);
    end = entryEnd.IsActive()
        ? entryEnd.EntryOffset() : start + entryStart.EntrySize();
}
