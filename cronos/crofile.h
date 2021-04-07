#ifndef __CROFILE_H
#define __CROFILE_H

#include "cronos_abi.h"
#include "crodata.h"
#include "croentry.h"
#include "croblock.h"
#include "crorecord.h"
#include <string>

enum crofile_status {
    CROFILE_OK = 0,
    CROFILE_ERROR,
    CROFILE_FOPEN,
    CROFILE_FREAD,
    CROFILE_HEADER,
    CROFILE_VERSION
};

class CroFile
{
public:
    CroFile(const std::wstring& path);
public:
    crofile_status Open();
    void Close();
    void Reset();
    void SetTableLimits(cronos_size tableLimit);

    inline const CronosABI* ABI() const
    {
        return m_pABI;
    }

    inline cronos_abi_num GetABIVersion() const
    {
        return m_pABI->GetABIVersion();
    }

    inline cronos_version GetVersion() const
    {
        return m_Version;
    }

    inline const std::wstring& GetPath() const { return m_Path; }
    inline crofile_status GetStatus() const { return m_Status; }
    inline const std::string& GetError() const { return m_Error; }
    crofile_status SetError(crofile_status st,
            const std::string& msg = "");
    crofile_status SetError(const std::string& msg = "");
    crofile_status UpdateError(crofile_status st,
            const std::string& msg = "");
    bool IsFailed() const;

    inline cronos_size GetDefaultBlockSize() const { return m_uDefLength;  }
    bool IsEncrypted() const;
    bool IsCompressed() const;

    void SetDefaultCrypt();
    void SetCryptKey(uint32_t secret, uint32_t serial = 1);
    inline const CroData& GetCryptTable() const { return m_Crypt; }
    void LoadCryptTable(CroData& key);
    void Decrypt(CroBuffer& data, uint32_t prefix, const CroData* crypt = NULL);

    bool IsEndOfEntries() const;

    bool IsValidOffset(cronos_off off, cronos_filetype type) const;
    FILE* FilePointer(cronos_filetype ftype) const;
    cronos_size FileSize(cronos_filetype ftype);
    cronos_off GetOffset(cronos_filetype ftype) const;
    void Seek(cronos_off off, cronos_filetype ftype);
    void Read(CroData& data, uint32_t count, cronos_size size);
    inline cronos_size DatFileSize() const { return m_DatSize; }
    inline cronos_size TadFileSize() const { return m_TadSize; }

    template<typename T = CroData>
    inline T Read(cronos_id id, uint32_t count, cronos_size size,
            cronos_filetype ftype)
    {
        T data = T();

        data.InitData(this, id, ftype, GetOffset(ftype), count*size);
        Read(data, count, size);

        return data;
    }

    template<typename T = CroData>
    inline T Read(cronos_id id, uint32_t count, cronos_size size,
            cronos_filetype ftype, cronos_off off)
    {
        T data = T();

        data.InitData(this, id, ftype, off, count*size);
        Read(data, count, size);

        return data;
    }

    template<typename T = CroData>
    inline T Read(cronos_id id, uint32_t count, cronos_value value)
    {
        const auto* i = ABI()->GetValue(value);
        return Read<T>(id, count, i->m_Size, i->m_FileType, i->m_Offset);
    }

    void LoadTable(cronos_filetype ftype, cronos_id id,
        cronos_size limit, CroTable& table);
    void LoadTable(cronos_filetype ftype, cronos_id id,
        cronos_off start, cronos_off end, CroTable& table);

    cronos_idx EntryCountFileSize() const;
    inline cronos_id IdEntryEnd() const
    {
        return (cronos_id)EntryCountFileSize();
    }

    cronos_idx OptimalEntryCount();
    CroEntryTable LoadEntryTable(cronos_id id, cronos_idx count);

    cronos_idx OptimalRecordCount(CroEntryTable* tad, cronos_id start);
    cronos_size BlockTableOffsets(CroEntryTable* tad,
        cronos_id id, cronos_idx count,
        cronos_off& start, cronos_off& end);
    CroBlockTable LoadBlockTable(CroEntryTable* tad,
        cronos_id id, cronos_idx count);

    CroRecordMap LoadRecordMap(cronos_id id, cronos_idx count);
private:
    std::wstring m_Path;

    crofile_status m_Status;
    std::string m_Error;

    FILE* m_fDat;
    FILE* m_fTad;

    cronos_size m_DatSize;
    cronos_size m_TadSize;

    cronos_size m_DatTableLimit;
    cronos_size m_TadTableLimit;

    bool m_bEOB;

    CronosABI* m_pABI;
    cronos_version m_Version;

    uint32_t m_uFlags;
    uint32_t m_uDefLength;
    cronos_size m_uTadRecordSize;

    CroData m_Crypt;
};

#endif
