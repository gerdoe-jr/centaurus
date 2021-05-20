#ifndef __CROFILE_H
#define __CROFILE_H

#include "cronos_abi.h"
#include "crodata.h"
#include "croentry.h"
#include "croblock.h"
#include "crorecord.h"
#include <memory>
#include <string>

#define CRONOS_DEFAULT_SERIAL 1

enum crofile_status {
    CROFILE_OK = 0,
    CROFILE_ERROR,
    CROFILE_FOPEN,
    CROFILE_FREAD,
    CROFILE_HEADER,
    CROFILE_VERSION
};

struct crofile_part {
    cronos_off m_Pos;
    cronos_size m_Size;
};

class CroFileRecord
{
public:
    inline void AddRecordPart(cronos_off off, cronos_size size)
    {
        m_Parts.emplace_back(off, size);
    }

    inline auto StartPart() const { return m_Parts.begin(); }
    inline auto EndPart() const { return m_Parts.end(); }

    inline cronos_size GetRecordSize() const
    {
        cronos_size size = 0;
        for (auto it = StartPart(); it != EndPart(); it++)
            size += it->m_Size;
        return size;
    }
private:
    std::vector<struct crofile_part> m_Parts;
};

class CroFile
{
public:
    CroFile(const std::wstring& path);

    crofile_status Open();
    void Close();
    void Reset();
    void SetTableLimits(cronos_size tableLimit);

    void SetupCrypt();
    void SetupCrypt(uint32_t secret, uint32_t serial);
    void LoadCrypt(CroData& key, unsigned keyLen = 8);
    void Decrypt(CroBuffer& block, uint32_t offset, const CroData* crypt = NULL);
    CroBuffer Decompress(CroBuffer& zbuffer);

    inline cronos_size GetDefaultBlockSize() const { return m_DefLength; }
    inline bool IsEncrypted() const { return m_bEncrypted; }
    inline bool IsCompressed() const { return m_bCompressed; }
    inline const CroData& GetSecret() const { return m_Secret; }
    inline const CroData& GetLiteSecret() const { return m_LiteSecret; }
    inline const CroData& GetCrypt() const { return m_Crypt; }
    inline CroData& GetCrypt() { return m_Crypt; }

    inline uint32_t GetSecretKey(const CroData& secret)
    {
        return secret.Get<uint32_t>((cronos_off)0x00);
    }

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

    bool IsEndOfEntries() const;
    bool IsValidOffset(cronos_off off, cronos_filetype type) const;
    FILE* FilePointer(cronos_filetype ftype) const;
    cronos_size FileSize(cronos_filetype ftype);
    cronos_off GetOffset(cronos_filetype ftype) const;
    void Seek(cronos_off off, cronos_filetype ftype);
    void Read(CroData& data, cronos_id id, cronos_filetype ftype,
        cronos_pos pos, cronos_size size, cronos_idx count = 1);
    void Read(CroData& data, cronos_id id, const cronos_abi_value* value,
        cronos_idx count = 1);
    inline cronos_size DatFileSize() const { return m_DatSize; }
    inline cronos_size TadFileSize() const { return m_TadSize; }

    template<typename T = CroData>
    inline T Read(cronos_id id, cronos_value value, uint32_t count = 1)
    {
        T data = T();
        
        Read(data, id, ABI()->GetValue(value), count);
        return data;
    }

    template<typename T = CroData>
    inline T Read(cronos_id id, cronos_filetype ftype,
        cronos_pos pos, cronos_size size)
    {
        T data = T();

        Read(data, id, ftype, pos, size);
        return data;
    }

    inline void LoadTable(cronos_filetype ftype, cronos_id id, cronos_pos pos,
        cronos_size size, cronos_idx count, CroTable& table)
    {
        Read(table, id, ftype, pos, size, count);
        table.Sync();
    }

    inline void LoadTable(cronos_filetype ftype, cronos_id id, cronos_pos pos,
        cronos_off end, CroTable& table)
    {
        Read(table, id, ftype, pos, end - pos, 1);
        table.Sync();
    }

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

    inline CroBlock ReadFirstBlock(cronos_pos pos)
    {
        CroBlock block = CroBlock(true);
        Read(block, CRONOS_FILE_ID, CRONOS_DAT, pos,
            ABI()->Size(cronos_first_block_hdr));
        return block;
    }

    inline CroBlock ReadNextBlock(cronos_pos pos)
    {
        CroBlock block = CroBlock(false);
        Read(block, CRONOS_FILE_ID, CRONOS_DAT, pos,
            ABI()->Size(cronos_block_hdr));
        return block;
    }

    inline cronos_pos EntryFileOffset(cronos_id id)
    {
        return ABI()->Offset(cronos_tad_entry)
            + (id - 1) * ABI()->Size(cronos_tad_entry);
    }

    inline CroEntry ReadFileEntry(cronos_id id)
    {
        return Read<CroEntry>(id, CRONOS_TAD,
            EntryFileOffset(id), ABI()->Size(cronos_tad_entry));
    }

    CroFileRecord ReadFileRecord(const CroEntry& entry);
    CroBuffer ReadRecord(cronos_id id, const CroFileRecord& rec);
    
    CroBuffer ReadRecord(cronos_id id);
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
    cronos_size m_uTadRecordSize;
    
    CroData m_Header;
    bool m_bEncrypted;
    bool m_bCompressed;
    cronos_size m_DefLength;
    
    CroData m_Secret;
    CroData m_LiteSecret;
    CroData m_Pad;
    CroData m_Crypt;
};

#endif
