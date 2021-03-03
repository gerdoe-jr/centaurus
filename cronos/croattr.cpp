#include "croattr.h"
#include <stdexcept>

/* CroAttr */

CroAttr::CroAttr()
    : m_bIsEntryId(false)
{
}

const std::string& CroAttr::GetName() const
{
    return m_AttrName;
}

CroBuffer& CroAttr::GetAttr()
{
    return m_Attr;
}

std::string CroAttr::GetString() const
{
    return std::string((const char*)m_Attr.GetData(), m_Attr.GetSize());
}

void CroAttr::Parse(CroStream& stream)
{
    uint8_t nameLen = stream.Read<uint8_t>();
    m_AttrName = std::string((const char*)stream.Read(nameLen), nameLen);

    uint32_t attrValue = stream.Read<uint32_t>();

    if (attrValue & 0x80000000)
    {
        uint32_t attrLen = attrValue & 0x7FFFFFFF;
        m_Attr.Copy(stream.Read(attrLen), attrLen);
        m_bIsEntryId = false;
    }
    else
    {
        m_Attr.Write((uint8_t*)&attrValue, sizeof(attrValue));
        m_bIsEntryId = true;
    }
}

/* CroField */

CroField::CroField()
{
}

const std::string& CroField::GetName() const
{
    return m_Name;
}

CroFieldType CroField::GetType() const
{
    return m_Type;
}

cronos_flags CroField::GetFlags() const
{
    return m_Flags;
}

cronos_idx CroField::Parse(CroStream& stream)
{
    stream.Read<uint16_t>(); // size
    m_Type = (CroFieldType)stream.Read<uint16_t>();
    cronos_idx index = stream.Read<uint32_t>();

    uint8_t nameLen = stream.Read<uint8_t>();
    m_Name = std::string((const char*)stream.Read(nameLen), nameLen);
    m_Flags = stream.Read<uint32_t>();

    if (stream.Read<uint8_t>())
    {
        stream.Read<uint32_t>(); // dataindex
        stream.Read<uint32_t>(); // datalength
    }

    return index;
}

/* CroBase */

CroBase::CroBase()
{
    m_BitcardId = INVALID_CRONOS_ID;
    m_Index = INVALID_CRONOS_ID;
    m_Flags = 0;
}

const std::string& CroBase::GetName() const
{
    return m_Name;
}

const CroField& CroBase::Field(cronos_idx idx) const
{
    auto it = m_Fields.find(idx);
    if (it == m_Fields.end())
        throw std::runtime_error("invalid field");
    return it->second;
}

cronos_idx CroBase::FieldCount() const
{
    return m_Fields.size();
}

void CroBase::Parse(CroStream& stream, bool hasPrefix)
{
    if (hasPrefix)
    {
        if (stream.Read<uint8_t>() != CROBASE_PREFIX)
            throw std::runtime_error("not a base prefix");
    }
    stream.Read<uint16_t>(); // vocflags
    stream.Read<uint16_t>(); // unk1
    if (stream.Read<uint16_t>() == CROBASE_LINKED)
        stream.Read<uint32_t>(); // linked data id
    m_BitcardId = stream.Read<uint32_t>();
    m_Index = stream.Read<uint32_t>();

    uint8_t nameLen = stream.Read<uint8_t>();
    m_Name = std::string((const char*)stream.Read(nameLen), nameLen);

    uint8_t mcLen = stream.Read<uint8_t>();
    m_Mnemocode = std::string((const char*)stream.Read(mcLen), mcLen);
    
    m_Flags = stream.Read<uint32_t>();
    
    uint32_t fieldNum = stream.Read<uint32_t>();
    for (unsigned i = 0; i < fieldNum; i++)
    {
        CroField field;
        cronos_idx index = field.Parse(stream);
        m_Fields.insert(std::make_pair(index, field));
    }
}
