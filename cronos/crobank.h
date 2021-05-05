#ifndef __CROBANK_H
#define __CROBANK_H

#include "crofile.h"
#include "crostru.h"
#include "croexception.h"

#define CRONOS_DEFAULT_CODEPAGE 1251

class CroParser;

using CroBaseIter = std::vector<CroBase>::iterator;
class CroBank
{
public:
    CroBank(const std::wstring& path);
    virtual ~CroBank();

    virtual bool Open();
    virtual void Close();
    virtual bool TryBankPath();
    
    void SetBankPath(const std::wstring& path);
    const std::wstring& GetBankPath() const;
    void SetTextCodePage(unsigned cp = CRONOS_DEFAULT_CODEPAGE);
    unsigned GetTextCodePage() const;

    virtual std::wstring GetWString(const uint8_t* str, cronos_size len);
    virtual std::string GetString(const uint8_t* str, cronos_size len);
    virtual CroFile* File(crobank_file file);
    const std::wstring& FileName(crobank_file file);

    inline unsigned BaseCount() const { return m_Bases.size(); }
    inline CroBaseIter StartBase() { return m_Bases.begin(); }
    inline CroBaseIter EndBase() { return m_Bases.end(); }
    CroBase* GetBaseByIndex(unsigned idx);

    CroParser* Parser();
    virtual void ParserStart(CroParser* parser);
    virtual void ParserEnd(CroParser* parser);

    virtual void OnCronosException(const CroException& exc);
    virtual void OnParseProp(CroProp& prop);
protected:
    std::wstring m_Path;
    unsigned m_TextCodePage;
private:
    std::unique_ptr<CroFile> m_CroFile[CROFILE_COUNT];
    CroParser* m_Parser;
public:
    uint32_t m_BankFormSaveVer;
    uint32_t m_BankId;
    std::wstring m_BankName;
    std::vector<CroBase> m_Bases;
    std::vector<CroBuffer> m_Formuls;
    uint32_t m_BankSerial;
    uint32_t m_BankCustomProt;
    std::wstring m_BankSysPass;
    int m_BankVersion;
};

enum crovalue_parse {
    CroRecord_Start,
    CroRecord_End,

    CroValue_Read,
    CroValue_Comp,
    CroValue_Multi,
    CroValue_Next,
};

#define CROVALUE_COMP   0x1B
#define CROVALUE_MULTI  0x1D
#define CROVALUE_SEP    0x1E

class CroBankParser : public CroParser
{
public:
    CroBankParser(CroBank* bank);
    void Parse(cronos_id id, CroBuffer& data);
    void Reset();

    inline cronos_id RecordId() const { return m_Id; }
    inline CroBase* IdentBase() const { return m_pBase; }
    inline bool IsLastField()
    {
        if (!m_pBase) return true;

        return std::next(m_FieldIter) == m_pBase->EndField();
    }

    uint8_t* Value();
    cronos_off ValueOff();
    cronos_size ValueSize();
    CroType ValueType();

    crovalue_parse ParseValue();

    CroIdent ReadIdent();
    CroInteger ReadInteger();
private:
    cronos_id m_Id;
    CroBuffer* m_pData;
    CroStream m_Record;

    CroBase* m_pBase;
    CroFieldIter m_FieldIter;

    cronos_off m_ValueOff;
    cronos_size m_ValueSize;
    CroType m_ValueType;
};

#endif