#ifndef __CENTAURUS_BANK_H
#define __CENTAURUS_BANK_H

#ifdef CENTAURUS_INTERNAL
#include "centaurus.h"
#include <crofile.h>
#include <croattr.h>
#include <utility>
#include <vector>
#include <string>

class CentaurusBank : public ICentaurusBank
{
public:
    CentaurusBank();
    virtual ~CentaurusBank();

    bool LoadPath(const std::wstring& path) override;
    CroFile* File(CroBankFile type) const override;

    void ExportHeaders() const override;

    void LoadBase(ICentaurusExport* exp, CroAttr& base);
    void ParseAttr(ICentaurusExport* exp, CroStream& stream, CroAttr& attr);
    
    void LoadStructure(ICentaurusExport* exp) override;
    void ExportStructure(ICentaurusExport* exp) override;
private:
    std::wstring m_Path;
    std::unique_ptr<CroFile> m_Files[CroBankFile_Count];

    std::vector<CroAttr> m_Attrs;
    std::vector<CroBuffer> m_Bases;
};
#endif

#endif