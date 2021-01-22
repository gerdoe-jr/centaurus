#ifndef __CRONOS_H
#define __CRONOS_H

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <tuple>

// 01.02    3x, small model
// 01.03    3x, big model
// 01.04    3x, lite
// 01.11    4x
// 01.13    4x, lite
// 01.14    4x, pro

// hh hh hh hh  hh hh hh ??
// ?? ?? vs vs  vs vs vs vs
// fl fl dl dl  .. .. .. ..

#define CRONOS_HEADER       "CroFile"
#define CRONOS_ENCRYPTION   (1<<0)
#define CRONOS_COMPRESSION  (1<<1)

// TAD V3

// of of of of  sz sz sz sz
// fl fl fl fl
#define TAD_V3_BASE         0x08
#define TAD_V3_SIZE         0x0C
#define TAD_V3_DELETED      0xFFFFFFFF
#define TAD_V3_FSIZE(off)   (off&0x7FFFFFFF);
#define TAD_V3_OFFSET(off)  (off&0x1FFFFFFF);

// TAD V4+

// rz of of of  of of of of
// sz sz sz sz  fl fl fl fl
#define TAD_V4_BASE         0x10
#define TAD_V4_SIZE         0x10
#define TAD_V4_RZ(off)      (off>>56)
#define TAD_V4_RZ_DELETED   (1<<1)
#define TAD_V4_FSIZE(off)   (off)
#define TAD_V4_OFFSET(off)  (off&0xFFFFFFFFFFFF)

typedef uint32_t record_id;
typedef uint32_t record_off;

typedef enum {
    CROFILE_OK = 0,
    CROFILE_ERROR,
    CROFILE_FOPEN,
    CROFILE_FREAD,
    CROFILE_HEADER,
    CROFILE_VERSION
} crofile_status;

class CroEntry
{
public:
    CroEntry();
    CroEntry(record_id id, int version);

    virtual record_id Id() const;
    virtual int Version() const;
    virtual bool IsVersion(int version) const;

    inline bool Is3() const { return IsVersion(3); }
    inline bool Is4() const { return IsVersion(4); }
    inline bool Is4A() const { return m_iVersion >= 4; }
protected:
    record_id m_Id;
    int m_iVersion;
};

class CroBuffer
{
public:
    CroBuffer();
    CroBuffer(uint8_t* data, uint32_t size);
    CroBuffer(CroBuffer&&);
    ~CroBuffer();

    bool IsEmpty() const;
    uint8_t* GetData() const;
    uint8_t* Dereference(uint32_t& size);
    inline uint32_t GetSize() { return m_uSize; }
    void Alloc(uint32_t size);
    void Write(uint8_t* data, uint32_t size);
private:
    uint8_t* m_pData;
    uint32_t m_uSize;
};

typedef enum {
    CROFILE_DAT,
    CROFILE_TAD
} crofile_type;

typedef enum {
    CROFILE_OFFSET_INVALID = -1,
    CROFILE_OFFSET_OK,
    CROFILE_OFFSET_OUTSIDE
} crofile_offset_type;

class CroFile;
class CroFileBuffer : public CroBuffer, public CroEntry
{
public:
    CroFileBuffer(CroFile& file, crofile_type type);
    CroFileBuffer(CroFileBuffer&& other);
    ~CroFileBuffer();

    inline CroFile& GetFile() const { return m_File; }

    void Assign(uint64_t startOff, uint64_t size,
            record_id idx = 0);
    void Read();

    uint64_t GetStartOffset() const;
    uint64_t GetEndOffset() const;

    uint8_t* GetFileData() const { return GetData(); }
    uint64_t GetBufferSize() const { return m_uBufferSize; }

    crofile_offset_type CheckOffset(uint64_t offset) const;
    uint8_t* DataPointer(uint64_t offset) const;
private:
    CroFile& m_File;
    crofile_type m_Type;

    uint64_t m_uFileOffset;
    uint64_t m_uBufferSize;
};

class RecordEntry : public CroEntry
{
public:
    RecordEntry();
    RecordEntry(record_id id, int version,
            uint8_t* entry);

    inline uint8_t* GetEntry() { return m_pEntry; }

    uint64_t GetOffset() const;
    uint32_t GetSize() const;
    uint32_t GetFlags() const;

    bool IsActive() const;
private:
    uint8_t* m_pEntry;
};

class CroEntryTable : public CroFileBuffer
{
public:
    CroEntryTable(CroFile& file, record_id idx,
            unsigned count);
    CroEntryTable(CroEntryTable& other);
protected:
    unsigned GetEntrySize() const;
    uint64_t CalcOffset(record_id idx) const;
public:

    void SetEntryCount(unsigned count);
    unsigned GetEntryCount() const;
    RecordEntry GetRecordEntry(record_off i);

    bool FirstActiveRecord(record_off i, RecordEntry& entry);
    bool RecordTableRange(record_off i, record_off* pStart,
            record_off* pEnd, unsigned sizeLimit);
};

class CroFile;
class CroRecordTable;

class CroRecord : public CroBuffer, public CroEntry
{
public:
    CroRecord(record_id i, CroEntryTable& entries,
            CroRecordTable& records);

    uint8_t* FirstBlock(uint32_t* pSize, uint64_t* pNext);

    record_off EntryOffset();
    record_off RecordOffset();

    uint64_t NextBlockOffset(uint8_t* block);
    uint32_t BlockSize(uint8_t* block);
    uint8_t* BlockData(uint8_t* block, bool first);

    void LoadBlocks(CroFile* file);
private:
    CroEntryTable& m_Entries;
    CroRecordTable& m_Records;
};

class CroRecordTable : public CroFileBuffer
{
public:
    CroRecordTable(CroFile& file, CroEntryTable& entries);

    void SetRange(record_off start, record_off end);
    void GetRange(record_off* pStart, record_off* pEnd) const;

    unsigned GetRecordCount() const;
private:
    CroEntryTable& m_Entries;
    record_off m_Start;
    record_off m_End;
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

    inline int GetMajor() const { return m_iMajor; }
    inline int GetMinor() const { return m_iMinor; }
    inline int GetVersion() const { return m_iVersion; }
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

    FILE* GetFile(crofile_type type) const;
    uint64_t GetFileSize(crofile_type type) const;
    uint64_t ReadBuffer(CroBuffer* out, uint64_t offset,
            crofile_type type);

    unsigned EstimateEntryCount() const;
    bool IsEndOfEntries() const;
    uint32_t GetOptimalEntryCount() const;
    CroEntryTable LoadEntryTable(record_id idx, unsigned burst);
    //BlockTable LoadBlockTable(RecordTable& record, record_id i);
private:
    std::wstring m_Path;

    crofile_status m_Status;
    std::string m_Error;

    FILE* m_fDat;
    FILE* m_fTad;

    uint64_t m_DatSize;
    uint64_t m_TadSize;

    int m_iMajor;
    int m_iMinor;

    int m_iVersion;
    uint32_t m_uFlags;
    uint32_t m_uDefLength;
    uint32_t m_uTadRecordSize;

    uint8_t* m_pCrypt;
    bool m_bEOB;
};

#endif
