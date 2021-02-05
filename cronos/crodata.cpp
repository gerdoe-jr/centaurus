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
    cronos_rel off, cronos_size size)
    : CroEntity(table.File(), id), CroBuffer(table.Data(off), size)
{
    m_FileType = table.GetFileType();
    m_uOffset = table.FileOffset(off);
}

CroData::CroData(const CroData& table, cronos_id id,
    cronos_rel off, cronos_size size)
{
    InitEntity(table.File(), id);
    InitBuffer((uint8_t*)table.Data(off), size, off);
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

cronos_pos CroData::GetStartOffset() const
{
    return m_uOffset;
}

cronos_pos CroData::GetEndOffset() const
{
    return GetStartOffset() + GetSize();
}

bool CroData::CheckValue(cronos_pos value) const
{
    return true;
}

crodata_offset_type CroData::CheckOffset(cronos_pos off) const
{
    if (!File()->IsValidOffset(off, m_FileType))
        return CRODATA_OFFSET_INVALID;
    else if (off < GetStartOffset())
        return CRODATA_OFFSET_BEHIND;
    else if (off >= GetEndOffset())
        return CRODATA_OFFSET_AHEAD;
    return CRODATA_OFFSET_OK;
}

bool CroData::IsValidOffset(cronos_pos off) const
{
    return CheckOffset(off) == CRODATA_OFFSET_OK;
}

bool CroData::OffsetBehind(cronos_pos off) const
{
    return CheckOffset(off) == CRODATA_OFFSET_BEHIND;
}

bool CroData::OffsetAhead(cronos_pos off) const
{
    return CheckOffset(off) == CRODATA_OFFSET_AHEAD;
}

cronos_off CroData::DataOffset(cronos_pos off) const
{
    if (!IsValidOffset(off))
        throw CroException(File(), "CroData::DataOffset invalid offset");
    return off - GetStartOffset();
}

cronos_off CroData::FileOffset(cronos_rel off) const
{
    if (off > GetSize())
        throw CroException(File(), "CroData::FileOffset", off);
    return GetStartOffset() + off;
}

const uint8_t* CroData::Data(cronos_rel off) const
{
    return GetData() + off;
}

uint8_t* CroData::Data(cronos_rel off)
{
    return GetData() + off;
}
