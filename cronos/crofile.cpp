#include "crofile.h"
#include "cronos_abi.h"
#include "cronos_format.h"
#include "cronos02.h"
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

/* CroFile */

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

    Reset();

    //Generic ABI
    m_pABI = CronosABI::GenericABI();
    m_Header = Read(CRONOS_FILE_ID, cronos_hdr);

    char szMajor[4] = {0};
    char szMinor[4] = {0};
    memcpy(szMajor, m_Header.Data(cronos_hdr_major), 2);
    memcpy(szMinor, m_Header.Data(cronos_hdr_minor), 2);

    uint16_t flags = m_Header.Get<uint16_t>(cronos_hdr_flags);
    m_bEncrypted = flags & CRONOS_ENCRYPTION;
    m_bCompressed = flags & CRONOS_COMPRESSION;

    uint16_t defLength = m_Header.Get<uint16_t>(cronos_hdr_deflength);
    m_DefLength = defLength;

    cronos_abi_num abiVersion = cronos_abi_version(
        atoi(szMajor), atoi(szMinor));
    m_pABI = CronosABI::GetABI(abiVersion);
    if (!ABI())
    {
        return SetError(CROFILE_VERSION, std::string("unknown ABI ")
            + szMajor + "." + szMinor);
    }

    m_Version = ABI()->GetVersion();
    m_uTadRecordSize = ABI()->Size(cronos_tad_entry);

    m_Secret = Read(CRONOS_FILE_ID, cronos_secret);
    if (ABI()->IsLite())
        m_LiteSecret = Read(CRONOS_FILE_ID, cronos_litesecret);
    if (m_Version == CRONOS3)
        m_Pad = Read(CRONOS_FILE_ID, cronos_pad);

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

void CroFile::SetupCrypt()
{
    CroData key;
    LoadCrypt(key);
}

void CroFile::SetupCrypt(uint32_t secret, uint32_t serial)
{
    CroData key;
    key.Write((uint8_t*)&serial, sizeof(uint32_t));
    key.Write((uint8_t*)&secret, sizeof(uint32_t));
    LoadCrypt(key);
}

void CroFile::LoadCrypt(CroData& key, unsigned keyLen)
{
    if (key.IsEmpty())
    {
        Read(m_Crypt, CRONOS_FILE_ID, &cronos02_crypt_table);
    }
    else
    {
        m_Crypt = Read(CRONOS_FILE_ID, cronos_crypt);

        auto bf = std::make_unique<blowfish_t>();
        blowfish_init(bf.get(), key.GetData(), keyLen);
        blowfish_decrypt_buffer(bf.get(), m_Crypt.GetData(),
            m_Crypt.GetSize());
    }
}

void CroFile::Decrypt(CroBuffer& block, uint32_t prefix, const CroData* crypt)
{
    if (!crypt) crypt = &m_Crypt;
    if (m_Crypt.IsEmpty())
    {
        SetError("Decrypt !m_Crypt");
        return;
    }

    uint8_t* pBlock = block.GetData();
    const uint8_t* pTable = GetVersion() <= 4
        ? crypt->GetData() + 0x100 : crypt->GetData();
    for (unsigned i = 0; i < block.GetSize(); i++)
        pBlock[i] = pTable[pBlock[i]] - (uint8_t)(i + prefix);
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

void CroFile::Read(CroData& data, cronos_id id, cronos_filetype ftype,
    cronos_pos pos, cronos_size size, cronos_idx count)
{
    if (ftype == CRONOS_TAD || ftype == CRONOS_DAT)
    {
        FILE* fp;
        cronos_size fileSize;

        if (ftype == CRONOS_TAD)
        {
            fp = m_fTad;
            fileSize = m_TadSize;
        }
        else
        {
            fp = m_fDat;
            fileSize = m_DatSize;
        }

        cronos_pos dataPos;
        cronos_size dataSize;

        if (data.GetFileType() == CRONOS_INVALID_FILETYPE)
        {
            dataPos = pos;
            dataSize = size;
        }
        else
        {
            dataPos = data.GetStartOffset();
            dataSize = data.IsEmpty() ? size : data.GetSize();
        }

        cronos_size totalSize = count * size;
        if (dataPos + totalSize > fileSize)
        {
            totalSize = fileSize - dataPos;
            count = totalSize / dataSize;
        }
        data.InitData(this, id, ftype, dataPos, totalSize);
        
        _fseeki64(fp, dataPos, SEEK_SET);
        cronos_idx read = (cronos_idx)fread(data.GetData(),
            dataSize, count, fp);

        if (read < count)
        {
            if (ferror(fp)) throw CroStdError(this);

            if (read) data.Alloc(read * dataSize);
            else throw CroException(this, "CroFile !fread");
        }

        m_bEOB = feof(fp);
    }
    else if (ftype == CRONOS_MEM)
    {
        throw CroException(this, "Read CRONOS_MEM with file pos");
    }
    else
    {
        throw CroException(this, "Invalid cronos file type");
    }
}

void CroFile::Read(CroData& data, cronos_id id, const cronos_abi_value* value,
    cronos_idx count)
{
    cronos_filetype ftype = value->m_FileType;
    if (ftype == CRONOS_MEM)
    {
        data.InitMemory(this, id, value);
    }
    else
    {
        Read(data, id, ftype, value->m_Offset, value->m_Size, count);
    }
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
    cronos_pos entryStart = ABI()->Offset(cronos_tad_entry)
        + (cronos_size)(id - 1) * entrySize;

    LoadTable(CRONOS_TAD, id, entryStart, entrySize, count, table);
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

cronos_size CroFile::BlockTableOffsets(CroEntryTable* tad, cronos_id id,
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

CroBlockTable CroFile::LoadBlockTable(CroEntryTable* tad,
    cronos_id id, cronos_idx count)
{
    cronos_off start, end;
    cronos_size size = BlockTableOffsets(tad, id, count, start, end);
    CroBlockTable table = CroBlockTable(*tad, id, count);

    LoadTable(CRONOS_DAT, id, start, end, table);
    table.SetEntryCount(count);
    return table;
}

CroRecordMap CroFile::LoadRecordMap(cronos_id id, cronos_idx count)
{
    CroRecordMap map;
    cronos_size entrySize = ABI()->Size(cronos_tad_entry);
    cronos_pos entryStart = ABI()->Offset(cronos_tad_entry)
        + (cronos_size)(id - 1) * entrySize;
    
    LoadTable(CRONOS_TAD, id, entryStart, entrySize, count, map);
    map.Load();

    return map;
}

