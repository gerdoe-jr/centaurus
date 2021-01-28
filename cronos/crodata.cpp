#include "crodata.h"
#include "crofile.h"
#include "croexception.h"

CroData::CroData()
{
}

CroData::CroData(CroFile* file, cronos_id id, cronos_filetype ftype,
        cronos_off off, cronos_size size)
    : CroEntity(file, id),
    m_FileType(ftype), m_uOffset(off)
{
    Alloc(size);
}

CroData::CroData(const CroData& table, cronos_id id,
        cronos_off rel, cronos_size size)
    : CroEntity(table.File(), id),
    CroBuffer(table.Data(rel), size)
{
    m_FileType = table.GetFileType();
    m_uOffset = table.FileOffset(rel);
}

void CroData::InitData(CroFile* file, cronos_id id, cronos_filetype ftype,
        cronos_off off, cronos_size size)
{
    InitEntity(file, id);
    Alloc(size);

    m_FileType = ftype;
    m_uOffset = off;
}

cronos_filetype CroData::GetFileType() const
{
    return m_FileType;
}

cronos_off CroData::GetStartOffset() const
{
    return m_uOffset;
}

cronos_off CroData::GetEndOffset() const
{
    return GetStartOffset() + GetSize();
}

crodata_offset_type CroData::CheckOffset(cronos_off off) const
{
    if (!File()->IsValidOffset(off, m_FileType))
        return CRODATA_OFFSET_INVALID;
    else if (off < GetStartOffset())
        return CRODATA_OFFSET_BEHIND;
    else if (off >= GetEndOffset())
        return CRODATA_OFFSET_AHEAD;
    return CRODATA_OFFSET_OK;
}

bool CroData::IsValidOffset(cronos_off off) const
{
    return CheckOffset(off) == CRODATA_OFFSET_OK;
}

bool CroData::OffsetBehind(cronos_off off) const
{
    return CheckOffset(off) == CRODATA_OFFSET_BEHIND;
}

bool CroData::OffsetAhead(cronos_off off) const
{
    return CheckOffset(off) == CRODATA_OFFSET_AHEAD;
}

cronos_off CroData::DataOffset(cronos_off off) const
{
    if (!IsValidOffset(off))
        throw CroException(File(), "CroData::DataOffset invalid offset");
    return off - GetStartOffset();
}

cronos_off CroData::FileOffset(cronos_off rel) const
{
    if (rel > GetSize())
        throw CroException(File(), "CroData::FileOffset rel > GetSize()");
    return GetStartOffset() + rel;
}

const uint8_t* CroData::Data(cronos_off rel) const
{
    return GetData() + rel;
}

uint8_t* CroData::Data(cronos_off rel)
{
    return GetData() + rel;
}
