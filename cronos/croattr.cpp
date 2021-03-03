#include "croattr.h"

CroAttr::CroAttr()
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
    m_bIsEntryId = !(attrValue & 0x80000000);

    if (IsEntryId())
        m_Attr.Write((uint8_t*)&attrValue, sizeof(attrValue));
    else
    {
        uint32_t attrLen = attrValue & 0x7FFFFFFF;
        m_Attr.Copy(stream.Read(attrLen), attrLen);
    }
}
