#ifndef __CENTAURUS_BANK_H
#define __CENTAURUS_BANK_H

#include "centaurus.h"
#include <crobank.h>
#include <croprop.h>
#include <utility>
#include <vector>
#include <string>

using BankProps = std::vector<CroAttr>;
class CentaurusBank : public CroBank, public ICentaurusBank
{
public:
    CentaurusBank();
    virtual ~CentaurusBank();

    std::wstring GetWString(const uint8_t* str, cronos_size len) override;
    std::string GetString(const uint8_t* str, cronos_size len) override;
    void BankCronosException(const CroException& exc) override;

    CroBank* Bank() override;

    void LoadStructure(ICronosAPI* cro) override;

    bool IsValidBase(unsigned index) const override;
    CroBase& Base(unsigned index) override;
    unsigned BaseEnd() const override;

    uint32_t BankId() const override;
    const std::wstring& BankName() const override;
private:
    uint32_t m_BankId;
    std::wstring m_BankName;

    std::map<cronos_idx, CroBase> m_Bases;
};

#endif