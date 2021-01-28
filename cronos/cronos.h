#ifndef __CRONOS_H
#define __CRONOS_H

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <tuple>

class CroFile;

/* CroData */
class CroData : public CroBuffer
{
public:
    inline CroFile* DataFile() const { return m_pFile; }
    cronos_id Id() const;
private:
    CroFile* m_pFile;
    cronos_id m_Id;
    cronos_off m_Off;
};

/* CroFileBuffer */

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

/* CroEntryTable */

class CroEntryTable : public CroFileBuffer
{
public:
    CroEntryTable(CroFile& file, record_id idx,
            unsigned count);
    //CroEntryTable(CroEntryTable& other);
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

/* CroRecordTable */
class CroRecord;

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

/* CroRecord */
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

#endif
