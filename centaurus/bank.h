#ifndef __CENTAURUS_BANK_H
#define __CENTAURUS_BANK_H

#include "centaurus.h"
#include <crobank.h>
#include <crofile.h>
#include <utility>
#include <vector>
#include <string>

using BankProps = std::vector<CroAttr>;
class CentaurusBank : public ICentaurusBank, public ICroBank
{
public:
    CentaurusBank();
    virtual ~CentaurusBank();

    bool Connect() override;
    void Disconnect() override;

    void AssociatePath(const std::wstring& path) override;
    std::wstring GetPath() const override;
    void SetCodePage(unsigned codepage) override;

    CroFile* File(CroBankFile type) const override;
    
    std::wstring BankWString(const uint8_t* str, cronos_size len) override;
    std::string BankString(const uint8_t* str, cronos_size len) override;
    CroFile* BankFile(crobank_file file) override;

    void LoadStructure(ICronosAPI* cro) override;
    
    BankProps LoadProps(CroRecordMap* stru);

    bool IsValidBase(unsigned index) const override;
    CroBase& Base(unsigned index) override;
    unsigned BaseEnd() const override;

    uint32_t BankId() const override;
    const std::wstring& BankName() const override;
private:
    std::wstring m_Path;
    unsigned m_uCodePage;
    std::unique_ptr<CroFile> m_Files[CroBankFile_Count];

    //std::map<std::string, CroBuffer> m_Attrs;
    
    uint32_t m_BankId;
    std::wstring m_BankName;

    std::map<cronos_idx, CroBase> m_Bases;
};

#endif