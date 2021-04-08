#ifndef __CENTAURUS_BANK_H
#define __CENTAURUS_BANK_H

#include "centaurus.h"
#include <crofile.h>
#include <croattr.h>
#include <utility>
#include <vector>
#include <string>

using BankProps = std::vector<CroAttr>;
class CentaurusBank : public ICentaurusBank, public ICroParser
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
    std::string String(const char* data, size_t len) override;
    CroFile* CroFileStru() override;
    CroFile* CroFileBank() override;
    CroFile* CroFileIndex() override;

    void ExportHeaders() const;

    void LoadBankInfo(ICentaurusLoader* cro) override;
    void LoadStructure(ICentaurusLoader* cro);

    BankProps LoadProps(CroRecordMap* stru);

    CroBuffer& Attr(const std::string& name) override;
    CroBuffer& Attr(unsigned index) override;
    unsigned AttrCount() const override;

    bool IsValidBase(unsigned index) const override;
    CroBase& Base(unsigned index) override;
    unsigned BaseEnd() const override;

    uint32_t BankId() const override;
    const std::wstring& BankName() const override;
private:
    std::wstring m_Path;
    unsigned m_uCodePage;
    std::unique_ptr<CroFile> m_Files[CroBankFile_Count];

    std::map<std::string, CroBuffer> m_Attrs;
    
    uint32_t m_BankId;
    std::wstring m_BankName;

    std::map<cronos_idx, CroBase> m_Bases;
};

#endif