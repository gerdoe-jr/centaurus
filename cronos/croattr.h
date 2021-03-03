#ifndef __CROATTR_H
#define __CROATTR_H

#include "crostream.h"
#include "crobuffer.h"
#include <string>
#include <map>

#define CROATTR_PREFIX 0x03

class CroAttr
{
public:
    CroAttr();

    const std::string& GetName() const;
    CroBuffer& GetAttr();
    std::string GetString() const;

    void Parse(CroStream& stream);
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

    cronos_idx Parse(CroStream& stream);
private:
    CroFieldType m_Type;
    
    std::string m_Name;
    cronos_flags m_Flags;
};

class CroBase
{
public:
    CroBase();

    const std::string& GetName() const;
    const CroField& Field(cronos_idx idx) const;
    cronos_idx FieldCount() const;

    void Parse(CroStream& stream, bool hasPrefix = true);

    inline cronos_idx Index() const { return m_Index; }
private:
    cronos_id m_BitcardId;
    cronos_idx m_Index;
    cronos_flags m_Flags;
    
    std::string m_Name;
    std::string m_Mnemocode;

    std::map<cronos_idx, CroField> m_Fields;
};

#endif