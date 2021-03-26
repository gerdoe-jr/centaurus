#ifndef __CROATTR_H
#define __CROATTR_H

#include "crostream.h"
#include "crobuffer.h"
#include <string>
#include <map>

class ICroParser
{
public:
    virtual std::string String(const char* data, size_t len) = 0;
};

#define CROATTR_PREFIX 0x03

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

    void Parse(ICroParser* parser, CroStream& stream);
    inline bool IsEntryId() const { return m_bIsEntryId; }
private:
    std::string m_AttrName;
    CroBuffer m_Attr;
    bool m_bIsEntryId;
};

#define CROBASE_PREFIX 0x04
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

    unsigned Parse(ICroParser* parser, CroStream& stream);
private:
    CroFieldType m_Type;
    unsigned m_uDataIndex;
    
    std::string m_Name;
    cronos_flags m_Flags;
};

class CroBase
{
public:
    CroBase();

    const std::string& GetName() const;
    const CroField& Field(unsigned idx) const;
    unsigned FieldCount() const;
    unsigned FieldEnd() const;
    
    unsigned Parse(ICroParser* parser, CroStream& stream,
        bool hasPrefix = true);
private:
    cronos_id m_BitcardId;
    cronos_flags m_Flags;
    
    std::string m_Name;
    std::string m_Mnemocode;

    std::map<unsigned, CroField> m_Fields;
};

#endif