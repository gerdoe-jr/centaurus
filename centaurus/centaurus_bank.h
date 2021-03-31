#ifndef __CENTAURUS_BANK_H
#define __CENTAURUS_BANK_H

#include "centaurus.h"
#include <crofile.h>
#include <croattr.h>
#include <utility>
#include <vector>
#include <string>

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

    void ExportHeaders() const override;

    void LoadStructure(ICentaurusExport* exp) override;
    void LoadBases(ICentaurusExport* exp) override;

    CroAttr& Attr(const std::string& name) override;
    CroAttr& Attr(unsigned index) override;
    unsigned AttrCount() const override;

    bool IsValidBase(unsigned index) const override;
    CroBase& Base(unsigned index) override;
    unsigned BaseEnd() const override;

    int BankId() const override;
    const std::wstring& BankName() const override;
private:
    std::wstring m_Path;
    unsigned m_uCodePage;
    std::unique_ptr<CroFile> m_Files[CroBankFile_Count];

    std::vector<CroAttr> m_Attrs;
    std::map<cronos_idx, CroBase> m_Bases;

    int m_iBankId;
    std::wstring m_BankName;

};

#endif