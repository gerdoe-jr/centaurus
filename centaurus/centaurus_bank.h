#ifndef __CENTAURUS_BANK_H
#define __CENTAURUS_BANK_H

#ifdef CENTAURUS_INTERNAL
#include "centaurus.h"
#include <crofile.h>
#include <utility>
#include <string>

class CentaurusBank : public ICentaurusBank
{
public:
    CentaurusBank();
    virtual ~CentaurusBank();

    bool LoadPath(const std::wstring& path) override;
    CroFile* File(CroBankFile type) const override;
    void ExportHeaders() const override;
private:
    std::wstring m_Path;
    std::unique_ptr<CroFile> m_Files[CroBankFile_Count];
};
#endif

#endif