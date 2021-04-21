#ifndef __CROBANK_H
#define __CROBANK_H

#include "crofile.h"
#include "crostru.h"
#include "croexception.h"

#define CRONOS_DEFAULT_CODEPAGE 1251

class CroParser;

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
    CroValue_Next,
    CroMulti_Next,
    CroRecord_End
};

class CroBankParser : public CroParser
{
public:
    CroBankParser(CroBank* bank);

    void Reset();
    void Parse(cronos_id id, CroBuffer& data);

    uint8_t* Value();
    cronos_size ValueSize();
    crovalue_parse ParseValue();
private:
    cronos_id m_Id;
    CroBuffer* m_pData;
    CroStream m_Record;

    CroBase* m_pBase;
    CroFieldIter m_FieldIter;

    cronos_off m_ValueOff;
    cronos_size m_ValueSize;
};

#endif