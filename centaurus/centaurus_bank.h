#ifndef __CENTAURUS_BANK_H
#define __CENTAURUS_BANK_H

#ifdef CENTAURUS_INTERNAL
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

    bool LoadPath(const std::wstring& path) override;
    CroFile* File(CroBankFile type) const override;
    void SetCodePage(unsigned codepage) override;
    
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

    uint64_t BankId() const override;
    const std::wstring& BankName() const override;
private:
    std::wstring m_Path;
    unsigned m_uCodePage;
    std::unique_ptr<CroFile> m_Files[CroBankFile_Count];

    std::vector<CroAttr> m_Attrs;
    std::map<cronos_idx, CroBase> m_Bases;

    uint64_t m_BankId;
    std::wstring m_BankName;

};
#endif

#endif