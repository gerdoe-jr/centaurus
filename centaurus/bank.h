#ifndef __CENTAURUS_BANK_H
#define __CENTAURUS_BANK_H

#include "centaurus.h"
#include <crostru.h>
#include <crobank.h>
#include <utility>
#include <vector>
#include <string>

class CroRecordMap;

class CentaurusBank : public CroBank, public ICentaurusBank
{
public:
    CentaurusBank();
    virtual ~CentaurusBank();

    std::wstring GetWString(const uint8_t* str, cronos_size len) override;
    std::string GetString(const uint8_t* str, cronos_size len) override;
    void OnCronosException(const CroException& exc) override;
    void OnParseProp(CroProp& prop) override;

    CroBank* Bank() override;
    CroFile* BankFile(crobank_file file) override;

    bool Connect() override;
    void Disconnect() override;

    void Load(ICronosAPI* cro) override;
    void LoadStructure();
    
    uint32_t BankFormSaveVer() const override;
    uint32_t BankId() const override;
    const std::wstring& BankName() const override;
    uint32_t BankSerial() const override;
    uint32_t BankCustomProt() const override;
    const std::wstring& BankSysPass() const override;
    int BankVersion() const override;

    bool IsValidBase(unsigned index) const override;
    CroBase& Base(unsigned index) override;
    unsigned BaseEnd() const override;
private:
    ICronosAPI* m_pCronos;
};

#endif