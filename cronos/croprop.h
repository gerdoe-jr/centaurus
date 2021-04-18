#ifndef __CROPROP_H
#define __CROPROP_H

#include "crobank.h"
#include "crostream.h"
#include <string>
#include <map>

#define CROPROP_REF 0x80000000
#define CROPROP_SIZE(value) (value&0x7FFFFFFF)
#define CROPROP_MIN_SIZE 0x0F

class CroProp
{
public:
    CroProp();

    const std::string& GetName() const;
    CroBuffer& GetProp();
    std::string GetString() const;
    
    inline bool IsRef() const { return !(m_Value & CROPROP_REF); }
    inline cronos_size PropSize() const { return CROPROP_SIZE(m_Value); }
    inline cronos_id RefBlockId() const { return m_Value; }

    void Parse(ICroBank* bank, CroStream& stream);
private:
    std::string m_Name;
    cronos_id m_Value;
    CroBuffer m_Prop;
};

#define CROBLOCK_PREFIX 0x04

#define CROBASE_LINKED 0x109

enum CroFieldType : uint16_t {
    Index,
    Number,
    String,
    Dict,
    Date,
    Time,
    File,
    Forward,
    Backward,
    ForwardBackward,
    Bind,
    Access,
    ExternalFile
};

class CroField
{
public:
    CroField();

    const std::string& GetName() const;
    CroFieldType GetType() const;
    cronos_flags GetFlags() const;

    void Parse(ICroBank* bank, CroStream& stream);
    
    CroFieldType m_Type;
    uint32_t m_Index;
    
    std::string m_Name;
    uint32_t m_Flags;

    uint32_t m_DataIndex;
    uint32_t m_DataLength;
};

class CroBase
{
public:
    CroBase();

    const std::string& GetName() const;
    const CroField& Field(unsigned idx) const;
    unsigned FieldCount() const;
    unsigned FieldEnd() const;
    
    void Parse(ICroBank* bank, CroProp& prop);
    
    uint16_t m_VocFlags;
    uint16_t m_BaseVersion;
    uint32_t m_BitcardId;
    uint32_t m_LinkedId;
    uint32_t m_BaseIndex;
    
    std::string m_Name;
    std::string m_Mnemocode;

    uint32_t m_Flags;
private:
    std::map<unsigned, CroField> m_Fields;
};

class CroPropNS
{
public:
    CroPropNS();

    inline uint32_t Serial() const { return m_Serial; }
    inline uint32_t CustomProt() const { return m_CustomProt; }
    inline std::string SysPassword() const { return m_SysPass; }
    inline bool IsPasswordSet() const { return !m_SysPass.empty(); }

    void Parse(ICroBank* bank, CroProp& prop);
private:
    uint32_t m_Serial;
    uint32_t m_CustomProt;
    std::string m_SysPass;
};

#endif