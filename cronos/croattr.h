#ifndef __CROATTR_H
#define __CROATTR_H

#include "crostream.h"
#include "crobuffer.h"
#include "crofile.h"
#include <string>
#include <map>

class ICroParser
{
public:
    virtual std::string String(const char* data, size_t len) = 0;
    virtual CroFile* CroFileStru() = 0;
    virtual CroFile* CroFileBank() = 0;
    virtual CroFile* CroFileIndex() = 0;
};

#define CROATTR_PREFIX 0x03
#define CROATTR_REF 0x80000000
#define CROATTR_SIZE(value) (value&0x7FFFFFFF)
#define CROATTR_MIN_SIZE 0x0F

#define CROATTR_REF_PREFIX 0x04

class CroAttr
{
public:
    CroAttr();

    const std::string& GetName() const;
    CroBuffer& GetAttr();
    std::string GetString() const;
    inline const char* String() const
    {
        return (const char*)m_Attr.GetData();
    }

    inline bool IsRef() const { return !(m_AttrValue & CROATTR_REF); }
    inline cronos_size AttrSize() const { return CROATTR_SIZE(m_AttrValue); }
    inline cronos_idx RefBlockId() const { return m_AttrValue; }

    void Parse(ICroParser* parser, CroStream& stream);
private:
    std::string m_AttrName;
    uint32_t m_AttrValue;
    CroBuffer m_Attr;
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

    void Parse(ICroParser* parser, CroStream& stream);
    
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
    
    void Parse(ICroParser* parser, CroAttr& attr);
    
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

class CroAttrNS
{
public:
    CroAttrNS();

    inline uint32_t BankSerial() const { return m_BankSerial; }
    inline uint32_t BankCustomProt() const { return m_BankCustomProt; }
    inline std::string BankSysPassword() const { return m_BankSysPass; }
    inline bool IsPasswordSet() const { return !m_BankSysPass.empty(); }

    void Parse(ICroParser* parser, CroAttr& attr);
private:
    uint32_t m_BankSerial;
    uint32_t m_BankCustomProt;
    std::string m_BankSysPass;
};

#endif