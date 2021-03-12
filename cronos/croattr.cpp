#include "croattr.h"
#include <stdexcept>
#include <algorithm>

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

void CroAttr::Parse(ICroParser* parser, CroStream& stream)
{
    uint8_t nameLen = stream.Read<uint8_t>();
    m_AttrName = parser->String((const char*)stream.Read(nameLen), nameLen);

    uint32_t attrValue = stream.Read<uint32_t>();

    if (attrValue & 0x80000000)
    {
        // block
        int32_t attrLen = (int32_t)(attrValue & 0x7FFFFFFF);
        m_Attr.Copy(stream.Read(attrLen), attrLen);
        m_bIsEntryId = false;
    }
    else
    {
        // ref block id
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

unsigned CroField::Parse(ICroParser* parser, CroStream& stream)
{
    uint16_t size = stream.Read<uint16_t>();
    cronos_rel pos = stream.GetPosition();

    m_Type = (CroFieldType)stream.Read<uint16_t>();
    unsigned index = stream.Read<uint32_t>();

    uint8_t nameLen = stream.Read<uint8_t>();
    m_Name = parser->String((const char*)stream.Read(nameLen), nameLen);
    m_Flags = stream.Read<uint32_t>();

    if (stream.Read<uint8_t>())
    {
        stream.Read<uint32_t>(); // dataindex
        stream.Read<uint32_t>(); // datalength
    }

    stream.SetPosition(pos + size);
    return index;
}

/* CroBase */

CroBase::CroBase()
{
    m_BitcardId = INVALID_CRONOS_ID;
    m_Flags = 0;
}

const std::string& CroBase::GetName() const
{
    return m_Name;
}

const CroField& CroBase::Field(unsigned idx) const
{
    auto it = m_Fields.find(idx);
    if (it == m_Fields.end())
        throw std::runtime_error("invalid field");
    return it->second;
}

unsigned CroBase::FieldCount() const
{
    return m_Fields.size();
}

unsigned CroBase::FieldEnd() const
{
    return m_Fields.empty() ? 0 : std::prev(m_Fields.end())->first;
}

cronos_idx CroBase::Parse(ICroParser* parser, CroStream& stream, bool hasPrefix)
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
    cronos_idx baseIndex = stream.Read<uint32_t>();

    uint8_t nameLen = stream.Read<uint8_t>();
    m_Name = parser->String((const char*)stream.Read(nameLen), nameLen);

    uint8_t mcLen = stream.Read<uint8_t>();
    m_Mnemocode = parser->String((const char*)stream.Read(mcLen), mcLen);
    
    m_Flags = stream.Read<uint32_t>();
    
    uint32_t fieldNum = stream.Read<uint32_t>();
    for (unsigned i = 0; i < fieldNum; i++)
    {
        CroField field;
        cronos_idx fieldIndex = field.Parse(parser, stream);
        m_Fields.insert(std::make_pair(fieldIndex, field));
    }

    return baseIndex;
}
