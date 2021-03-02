#include "centaurus_bank.h"
#include <stdexcept>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
namespace sc = boost::system;

CCentaurusBank::CCentaurusBank()
{
}

CCentaurusBank::~CCentaurusBank()
{
    CroFile* stru = File(CroStru);
    CroFile* bank = File(CroBank);
    CroFile* index = File(CroIndex);

    if (stru) stru->Close();
    if (bank) bank->Close();
    if (index) index->Close();
}

bool CCentaurusBank::LoadPath(const std::wstring& path)
{
    m_Path = path;

    m_Files[CroStru] = std::make_unique<CroFile>(m_Path + L"\\CroStru");
    m_Files[CroBank] = std::make_unique<CroFile>(m_Path + L"\\CroBank");
    m_Files[CroIndex] = std::make_unique<CroFile>(m_Path + L"\\CroIndex");

    for (unsigned i = 0; i < CroBankFile_Count; i++)
    {
        CroFile* file = File((CroBankFile)i);
        if (!file) continue;

        try {
            crofile_status st = file->Open();
            if (st != CROFILE_OK)
            {
                throw std::runtime_error("CroFile status "
                    + std::to_string(st));
            }
        }
        catch (const std::exception& e) {
            fwprintf(stderr, L"CroFile(%s): %S\n",
                file->GetPath().c_str(), e.what());
            m_Files[(CroBankFile)i] = NULL;
        }
    }

    return File(CroStru) || File(CroBank) || File(CroIndex);
}

CroFile* CCentaurusBank::File(CroBankFile type) const
{
    return !m_Files[type] ? NULL : m_Files[type].get();
}

void CCentaurusBank::ExportHeaders() const
{
    sc::error_code ec;
    std::wstring headerPath = m_Path + L"\\include";

    fs::create_directory(headerPath, ec);
    if (ec) throw std::runtime_error("ExportHeaders !create_directory");

    for (unsigned i = 0; i < CroBankFile_Count; i++)
    {
        CroFile* file = File((CroBankFile)i);
        if (!file) continue;

        FILE* fHdr = NULL;
        switch (i)
        {
        case CroStru:
            _wfopen_s(&fHdr, (headerPath + L"\\CroStru.h").c_str(), L"w");
            break;
        case CroBank:
            _wfopen_s(&fHdr, (headerPath + L"\\CroBank.h").c_str(), L"w");
            break;
        case CroIndex:
            _wfopen_s(&fHdr, (headerPath + L"\\CroIndex.h").c_str(), L"w");
            break;
        }

        if (!fHdr) throw std::runtime_error("ExportHeaders !fHdr");
        centaurus->ExportABIHeader(file->ABI(), fHdr);
        fclose(fHdr);
    }
}
