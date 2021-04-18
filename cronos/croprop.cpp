﻿#include "croprop.h"
#include "cronos02.h"
#include <stdexcept>
#include <algorithm>

/* CroProp */

CroProp::CroProp()
{
    m_Value = 0;
}

const std::string& CroProp::GetName() const
{
    return m_Name;
}

CroBuffer& CroProp::GetProp()
{
    return m_Prop;
}

std::string CroProp::GetString() const
{
    return std::string((const char*)m_Prop.GetData(), m_Prop.GetSize());
}

void CroProp::Parse(ICroBank* bank, CroStream& stream)
{
    uint8_t nameLen = stream.Read<uint8_t>();
    m_Name = bank->String((const char*)stream.Read(nameLen), nameLen);
    m_Value = stream.Read<uint32_t>();
}

/* CroField */

CroField::CroField()
{
    m_Type = Index;
    m_Index = 0;
    m_Flags = 0;
    m_DataIndex = 0;
    m_DataLength = 0;
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

void CroField::Parse(ICroBank* bank, CroStream& stream)
{
    uint16_t size = stream.Read<uint16_t>();
    cronos_rel pos = stream.GetPosition();

    m_Type = (CroFieldType)stream.Read<uint16_t>();
    m_Index = stream.Read<uint32_t>();

    uint8_t nameLen = stream.Read<uint8_t>();
    m_Name = bank->String((const char*)stream.Read(nameLen), nameLen);
    
    m_Flags = stream.Read<uint32_t>();
    if (stream.Read<uint8_t>())
    {
        m_DataIndex = stream.Read<uint32_t>();
        m_DataLength = stream.Read<uint32_t>();
    }

    stream.SetPosition(pos + size);
}

/* CroBase */

CroBase::CroBase()
{
    m_VocFlags = 0;
    m_BaseVersion = 0;
    m_BitcardId = 0;
    m_LinkedId = 0;
    m_BaseIndex = 0;
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

void CroBase::Parse(ICroBank* bank, CroProp& prop)
{
    CroStream base = CroStream(prop.GetProp());

    m_VocFlags = base.Read<uint16_t>(); // vocflags
    base.Read<uint16_t>(); // unk1
    m_BaseVersion = base.Read<uint16_t>();
    m_BitcardId = base.Read<uint32_t>();
    if (m_BaseVersion == CROBASE_LINKED)
        m_LinkedId = base.Read<uint32_t>();
    m_BaseIndex = base.Read<uint32_t>();

    uint8_t nameLen = base.Read<uint8_t>();
    m_Name = bank->String((const char*)base.Read(nameLen), nameLen);

    uint8_t mcLen = base.Read<uint8_t>();
    m_Mnemocode = bank->String((const char*)base.Read(mcLen), mcLen);
    
    m_Flags = base.Read<uint32_t>();
    uint32_t fieldNum = base.Read<uint32_t>();
    for (unsigned i = 0; i < fieldNum; i++)
    {
        CroField field;
        field.Parse(bank, base);
        m_Fields.insert(std::make_pair(i, field));
    }
}

/* CroPropNS */

CroPropNS::CroPropNS()
{
    m_Serial = 0;
    m_CustomProt = 0;
}

void CroPropNS::Parse(ICroBank* bank, CroProp& prop)
{
    CroFile* stru = bank->CroFileStru();
    CroData crypt = CroData(stru, INVALID_CRONOS_ID,
        cronos02_crypt_table.m_pMem, cronos02_crypt_table.m_Size);

    CroBuffer in = prop.GetProp();
    CroBuffer out;

    CroStream ns(in);
    ns.Read<uint8_t>();
    uint8_t prefix = ns.Read<uint8_t>();

    out.Write(ns.Read(12), 12);
    stru->Decrypt(out, prefix, &crypt);
    CroStream hdr(out);

    m_Serial = hdr.Read<uint32_t>();
    m_CustomProt = hdr.Read<uint32_t>();
    
    uint32_t passLen = hdr.Read<uint32_t>();
    if (passLen)
    {
        CroBuffer pass;
        pass.Write(ns.Read(passLen), passLen);
        stru->Decrypt(pass, prefix + 0x0C, &crypt);
        m_SysPass = bank->String((const char*)pass.GetData(), passLen);
    }
}
