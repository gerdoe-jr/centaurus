#ifndef __CROSTRU_H
#define __CROSTRU_H

#include "crotype.h"
#include "croparser.h"
#include "crorecord.h"
#include <string>
#include <map>

#define CROATTR_PREFIX         0x03

#define CROATTR_MIN_SIZE       0x0F
#define CROATTR_BANK           "Bank"

class CroAttr : public ICroParsee
{
public:
    virtual void Parse(CroParser* parser, CroStream& stream);

    inline const std::string& AttrName() const { return m_Name; }
private:
    std::string m_Name;
};

#define CROPROP_PREFIX          0x04

#define CROPROP_REF             0x80000000
#define CROPROP_SIZE(value)     (value&0x7FFFFFFF)
#define CROPROP_MIN_SIZE        0x0F

#define CROPROP_BANK            "Bank"
#define CROPROP_BANKFORMSAVEVER "BankFormSaveVer"
#define CROPROP_BANKID          "BankId"
#define CROPROP_BANKNAME        "BankName"
#define CROPROP_BASE_PREFIX     "Base"
#define CROPROP_FORMULA_PREFIX  "Formuls"
#define CROPROP_NS1             "NS1"
#define CROPROP_VERSION         "Version"

class CroProp : public ICroParsee
{
public:
    CroProp();

    virtual void Parse(CroParser* parser, CroStream& stream);

    const std::string& GetName() const;
    CroBuffer& Prop();
    std::string GetString() const;

    inline bool IsRef() const { return !(m_Value & CROPROP_REF); }
    inline cronos_size PropSize() const { return CROPROP_SIZE(m_Value); }
    inline cronos_id RefBlockId() const { return m_Value; }
private:
    std::string m_Name;
    cronos_id m_Value;
    CroBuffer m_Prop;
};

#define CROBLOCK_PREFIX         0x04

#define CROFIELD_SYSNUM         (1<<0)
#define CROFIELD_COMPONENT      (1<<13)

class CroField : public ICroParsee
{
public:
    CroField();

    void Parse(CroParser* parser, CroStream& stream) override;

    const std::string& GetName() const;
    CroType GetType() const;
    cronos_flags GetFlags() const;

    CroType m_Type;
    uint32_t m_Index;

    std::string m_Name;
    uint32_t m_Flags;

    uint32_t m_DataIndex;
    uint32_t m_DataLength;
};

#define CROBASE_LINKED          0x109

using CroFieldIter = std::vector<CroField>::iterator;
class CroBase : public ICroParsee
{
public:
    CroBase();

    void Parse(CroParser* parser, CroStream& stream) override;

    const std::string& GetName() const;
    
    inline CroFieldIter StartField() { return m_Fields.begin(); }
    inline CroFieldIter EndField() { return m_Fields.end(); }
    inline unsigned FieldCount() { return m_Fields.size(); }
    CroField* GetFieldByIndex(unsigned idx);
    CroField* GetFieldByDataIndex(unsigned idx);

    uint16_t m_VocFlags;
    uint16_t m_BaseVersion;
    uint32_t m_BitcardId;
    uint32_t m_LinkedId;
    uint32_t m_BaseIndex;

    std::string m_Name;
    std::string m_Mnemocode;

    uint32_t m_Flags;
private:
    std::vector<CroField> m_Fields;
};

class CroPropNS : public ICroParsee
{
public:
    CroPropNS();

    void Parse(CroParser* parser, CroStream& stream) override;

    inline uint32_t Serial() const { return m_Serial; }
    inline uint32_t CustomProt() const { return m_CustomProt; }
    inline std::string SysPassword() const { return m_SysPass; }
    inline bool IsPasswordSet() const { return !m_SysPass.empty(); }
private:
    uint32_t m_Serial;
    uint32_t m_CustomProt;
    std::string m_SysPass;
};

enum croblock_type : uint8_t {
    CROBLOCK_ATTR = CROATTR_PREFIX,
    CROBLOCK_PROP = CROPROP_PREFIX
};

class CroStruParser : public CroParser
{
public:
    CroStruParser(CroBank* bank, CroRecordMap* stru);

    inline CroRecordMap* CroStruMap() { return m_pStru; }

    croblock_type GetBlockType(cronos_id blockId);
    bool GetAttrByName(const std::string& name,
        CroBuffer& record, CroStream& stream);
    
    bool LoadBankProps();
private:
    CroRecordMap* m_pStru;
};

#endif
