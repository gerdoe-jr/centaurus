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

#ifndef WIN32
#define _fseeki64 fseeko64
#define _ftelli64 ftello64

#include "win32util.h"

static FILE* _wfopen(const wchar_t* path, const wchar_t* mode)
{
    return fopen(WcharToText(path).c_str(), WcharToText(mode).c_str());
}

#endif

CroFile::CroFile(const std::wstring& path)
    : m_Path(path)
{
    m_Version = INVALID_CRONOS_VERSION;

    SetError(CROFILE_OK);
    m_fDat = NULL;
    m_fTad = NULL;
    m_bEOB = false;
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

    m_DatSize = FileSize(CRONOS_DAT);
    m_TadSize = FileSize(CRONOS_TAD);

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

    m_uFlags = hdr.Get<uint16_t>(cronos_hdr_flags);
    m_uDefLength = hdr.Get<uint16_t>(cronos_hdr_deflength);
    m_uTadRecordSize = ABI()->Size(cronos_tad_entry);

    if (IsEncrypted())
    {
        if (m_Secret.IsEmpty())
        {
            m_Secret = hdr.CopyValue(cronos_hdr_secret);

            if (ABI()->IsLite())
            {
                CroData lite = hdr.CopyValue(cronos_hdrlite_secret);

                auto bf = std::make_unique<blowfish_t>();
                blowfish_init(bf.get(), m_Secret.GetData(), 4);
                blowfish_decrypt_buffer(bf.get(),
                    lite.GetData(), lite.GetSize());

                m_Secret.Copy(lite.GetData(), lite.GetSize());
            }
            
            memmove(m_Secret.GetData() + 4, m_Secret.GetData(), 4);
            memcpy(m_Secret.GetData(), &m_uSerial, 4);
        }

        m_Crypt = hdr.CopyValue(cronos_hdr_crypt);
        if (ABI()->GetModel() != cronos_model_small)
        {
            auto bf = std::make_unique<blowfish_t>();
            blowfish_init(bf.get(), m_Secret.GetData(), m_Secret.GetSize());
            blowfish_decrypt_buffer(bf.get(), m_Crypt.GetData(),
                m_Crypt.GetSize());
        }
    }

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

void CroFile::SetTableLimits(cronos_size tableLimit)
{
    m_DatTableLimit = tableLimit / 2 + tableLimit / 4;
    m_TadTableLimit = tableLimit / 4;
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

void CroFile::SetSecret(uint32_t serial, uint32_t key)
{
    if (m_Secret.IsEmpty())
        m_Secret.Alloc(8);

    *(uint32_t*)m_Secret.Data(0x00) = serial;
    *(uint32_t*)m_Secret.Data(0x04) = key;
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

bool CroFile::IsEndOfEntries() const
{
    return m_bEOB || feof(m_fTad);
}

bool CroFile::IsValidOffset(cronos_off off, cronos_filetype type) const
{
    if (off == INVALID_CRONOS_OFFSET)
        return false;

    switch (type)
    {
        case CRONOS_TAD: return off < m_TadSize;
        case CRONOS_DAT: return off < m_DatSize;
    }

    return false;
}

FILE* CroFile::FilePointer(cronos_filetype ftype) const
{
    return ftype == CRONOS_TAD ? m_fTad : m_fDat;
}

cronos_size CroFile::FileSize(cronos_filetype ftype)
{
    FILE* fp = FilePointer(ftype);
    
    _fseeki64(fp, 0L, SEEK_END);
    cronos_size size = (cronos_size)_ftelli64(fp);
    _fseeki64(fp, 0L, SEEK_SET);

    return size;
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
    
    cronos_size fileSize = data.GetFileType() == CRONOS_DAT
        ? m_DatSize : m_TadSize;

    size_t read = fread(data.GetData(),
        std::min(size, fileSize), count, fp);
    if (read < count)
    {
        if (ferror(fp)) throw CroStdError(this);
        
        if (read) data.Alloc(read * size);
        else throw CroException(this, "CroFile !fread");
    }

    m_bEOB = feof(fp);
}

void CroFile::LoadTable(cronos_filetype ftype, cronos_id id,
    cronos_size limit, CroTable& table)
{
    table.InitEntity(this, id);

    if (!IsValidOffset(table.GetStartOffset(), ftype))
        return;

    table.InitTable(this, ftype, id, limit);
    Read(table, table.GetEntryCount(), table.GetEntrySize());
    table.Sync();
}

void CroFile::LoadTable(cronos_filetype ftype, cronos_id id,
    cronos_off start, cronos_off end, CroTable& table)
{
    table.InitEntity(this, id);

    end = std::min(end, ftype == CRONOS_TAD
        ? m_TadSize : m_DatSize);
    cronos_size size = end - start;

    if (!IsValidOffset(start, ftype))
        return;

    table.InitTable(this, ftype, id, size);
    Read(table, 1, size);
    table.Sync();
}

cronos_idx CroFile::OptimalEntryCount()
{
    cronos_off offset = GetOffset(CRONOS_TAD);
    if (!offset)
        offset = ABI()->Offset(cronos_tad_entry);
    cronos_size remaining = std::min(m_TadTableLimit,
        m_TadSize - offset);
    return remaining / ABI()->Size(cronos_tad_entry);
}

CroEntryTable CroFile::LoadEntryTable(cronos_id id, cronos_idx count)
{
    CroEntryTable table;
    cronos_size entrySize = ABI()->Size(cronos_tad_entry);

    table.SetOffset(ABI()->Offset(cronos_tad_entry)
        + (cronos_size)(id - 1) * entrySize);
    LoadTable(CRONOS_TAD, id, count * entrySize, table);
    return table;
}

cronos_idx CroFile::EntryCountFileSize() const
{
    cronos_size entrySize = ABI()->Size(cronos_tad_size);
    cronos_off entryBase = ABI()->Offset(cronos_tad_entry);
    if ((cronos_off)m_TadSize < entryBase)
        return 0;
    return (m_TadSize - (cronos_off)entryBase) / entrySize;
}

cronos_idx CroFile::OptimalRecordCount(CroEntryTable* tad, cronos_id start)
{
    CroEntry entry;

    tad->FirstActiveEntry(start, entry);
    cronos_size tableSize = entry.EntrySize();

    while (tableSize < m_DatTableLimit)
    {
        if (!entry.IsActive())
            continue;
        if (entry.Id() + 1 == tad->IdEnd())
            break;
        
        CroEntry nextEntry = tad->GetEntry(entry.Id() + 1);
        if (!nextEntry.IsActive())
        {
            if (!tad->FirstActiveEntry(nextEntry.Id(), nextEntry))
                break;
        }
        if (nextEntry.EntryOffset() < entry.EntryOffset())
            break;
        
        tableSize += nextEntry.EntryOffset() - entry.EntryOffset();
        entry = tad->GetEntry(nextEntry.Id());
    }

    return entry.Id() - start + 1;
}

cronos_size CroFile::RecordTableOffsets(CroEntryTable* tad, cronos_id id,
    cronos_idx count, cronos_off& start, cronos_off& end)
{
    CroEntry entryStart, entryEnd;
    if (!tad->FirstActiveEntry(id, entryStart))
        throw CroException(this, "no active entry");
    start = entryStart.EntryOffset();

    entryEnd = tad->GetEntry(id + count - 1);
    end = entryEnd.IsActive()
        ? entryEnd.EntryOffset() + entryEnd.EntrySize() : m_DatSize;

    return end - start;
}

CroRecordTable CroFile::LoadRecordTable(CroEntryTable* tad,
    cronos_id id, cronos_idx count)
{
    cronos_off start, end;
    cronos_size size = RecordTableOffsets(tad, id, count, start, end);
    CroRecordTable table = CroRecordTable(*tad, id, count);

    table.SetOffset(start);
    LoadTable(CRONOS_DAT, id, start, end, table);
    table.SetEntryCount(count);

    return table;
}
