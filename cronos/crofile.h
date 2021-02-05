#ifndef __CROFILE_H
#define __CROFILE_H

#include "cronos_abi.h"
#include "crodata.h"
#include "crotable.h"
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
protected:
    bool IsSupported(int major, int minor);
public:
    crofile_status Open();
    void Close();
    void Reset();

    inline const CronosABI* ABI() const
    {
        return m_pABI;
    }

    inline const cronos_abi_num& GetABIVersion() const
    {
        return ABI()->Number();
    }

    inline cronos_version GetVersion() const
    {
        return m_Version;
    }

    inline crofile_status GetStatus() const { return m_Status; }
    inline const std::string& GetError() const { return m_Error; }
    crofile_status SetError(crofile_status st,
            const std::string& msg = "");
    crofile_status SetError(const std::string& msg = "");
    crofile_status UpdateError(crofile_status st,
            const std::string& msg = "");
    bool IsFailed() const;

    bool IsEncrypted() const;
    bool IsCompressed() const;

    void Decrypt(uint8_t* pBlock, unsigned size,
            uint32_t offset = 0);

    unsigned EstimateEntryCount() const;
    bool IsEndOfEntries() const;
    uint32_t GetOptimalEntryCount() const;

    bool IsValidOffset(cronos_off off, cronos_filetype type) const;
    FILE* FilePointer(cronos_filetype ftype) const;
    cronos_off GetOffset(cronos_filetype ftype) const;
    void Seek(cronos_off off, cronos_filetype ftype);
    void Read(CroData& data, uint32_t count, cronos_size size);

    template<typename T = CroData>
    inline T Read(cronos_id id, uint32_t count, cronos_size size,
            cronos_filetype ftype)
    {
        T data;

        data.InitData(this, id, ftype, GetOffset(ftype), count*size);
        Read(data, count, size);

        return data;
    }

    template<typename T = CroData>
    inline T Read(cronos_id id, uint32_t count, cronos_size size,
            cronos_filetype ftype, cronos_off off)
    {
        T data;

        data.InitData(this, id, ftype, off, count*size);
        Read(data, count, size);

        return data;
    }

    CroEntryTable LoadEntryTable(cronos_id id, unsigned burst);
    //CroEntryTable LoadEntryTable(record_id idx, unsigned burst);
    //BlockTable LoadBlockTable(RecordTable& record, record_id i);
private:
    std::wstring m_Path;

    crofile_status m_Status;
    std::string m_Error;

    FILE* m_fDat;
    FILE* m_fTad;

    cronos_size m_uDatSize;
    cronos_size m_uTadSize;

    bool m_bEOB;

    const CronosABI* m_pABI;
    cronos_version m_Version;

    uint32_t m_uFlags;
    uint32_t m_uDefLength;
    cronos_size m_uTadRecordSize;

    CroData m_Crypt;
};

#endif
