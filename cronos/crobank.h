#ifndef __CROBANK_H
#define __CROBANK_H

#include "crofile.h"
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

    virtual void ParserStart(CroParser* parser);
    virtual void ParserEnd(CroParser* parser);
protected:
    virtual void BankCronosException(const CroException& exc);

    std::wstring m_Path;
    unsigned m_TextCodePage;
private:
    std::unique_ptr<CroFile> m_CroFile[CROFILE_COUNT];
};

#endif