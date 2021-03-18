#include "centaurus_bank.h"
#include "centaurus_export.h"
#include "croattr.h"
#include "win32util.h"
#include <stdexcept>
#include <fstream>

#include <boost/json.hpp>
#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
namespace sc = boost::system;

#ifndef WIN32
#include <win32util.h>

static FILE* _wfopen(const wchar_t* path, const wchar_t* mode)
{
    return fopen(WcharToText(path).c_str(), WcharToText(mode).c_str());
}

static int _wfopen_s(FILE** fpFile, const wchar_t* path, const wchar_t* mode)
{
    *fpFile = fopen(WcharToText(path).c_str(), WcharToText(mode).c_str());
    return !!(*fpFile);
}

#endif

CentaurusBank::CentaurusBank()
{
    m_uCodePage = 1251;
}

CentaurusBank::~CentaurusBank()
{
    CroFile* stru = File(CroStru);
    CroFile* bank = File(CroBank);
    CroFile* index = File(CroIndex);

    if (stru) stru->Close();
    if (bank) bank->Close();
    if (index) index->Close();
}

bool CentaurusBank::LoadPath(const std::wstring& path)
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

CroFile* CentaurusBank::File(CroBankFile type) const
{
    return !m_Files[type] ? NULL : m_Files[type].get();
}

void CentaurusBank::SetCodePage(unsigned codepage)
{
    m_uCodePage = codepage;
}

std::string CentaurusBank::String(const char* data, size_t len)
{
    return WcharToText(AnsiToWchar(std::string(data, len), m_uCodePage));
}

void CentaurusBank::ExportHeaders() const
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

void CentaurusBank::LoadStructure(ICentaurusExport* exp)
{
    CroFile* stru = File(CroStru);
    if (!stru) throw std::runtime_error("no structure");

    CroBuffer bankStruct;
    exp->ReadRecord(stru, 1, bankStruct);
    
    CroStream stream = CroStream(bankStruct);
    if (stream.Read<uint8_t>() != CROATTR_PREFIX)
        throw std::runtime_error("not an attr prefix");

    while (!stream.IsOverflowed())
    {
        CroAttr attr;
        attr.Parse(this, stream);
        m_Attrs.push_back(attr);
    }

    m_BankId = atoi(Attr("BankId").String());
    m_BankName = AnsiToWchar(Attr("BankName").GetString());
}

void CentaurusBank::LoadBases(ICentaurusExport* exp)
{
    for (auto& attr : m_Attrs)
    {
        std::string attrName = attr.GetName();
        if (!attrName.starts_with("Base") || attrName.size() != 7)
            continue;

        CroBuffer rec;
        if (attr.IsEntryId())
        {
            uint32_t id = *(uint32_t*)attr.GetAttr().GetData();
            exp->ReadRecord(File(CroStru), id, rec);
        }
        else rec = CroBuffer(attr.GetAttr());

        CroStream stream = CroStream(rec);
        CroBase base;

        cronos_idx index = base.Parse(this, stream, attr.IsEntryId());
        m_Bases.insert(std::make_pair(index, base));
    }
}

CroAttr& CentaurusBank::Attr(const std::string& name)
{
    for (auto& attr : m_Attrs)
    {
        if (attr.GetName() == name)
            return attr;
    }

    throw std::runtime_error("invalid attribute");
}

CroAttr& CentaurusBank::Attr(unsigned index)
{
    return m_Attrs.at(index);
}

unsigned CentaurusBank::AttrCount() const
{
    return m_Attrs.size();
}

bool CentaurusBank::IsValidBase(unsigned index) const
{
    return m_Bases.find(index) != m_Bases.end();
}

CroBase& CentaurusBank::Base(unsigned index)
{
    auto it = m_Bases.find(index);
    if (it == m_Bases.end())
        throw std::runtime_error("invalid base");;
    return it->second;
}

unsigned CentaurusBank::BaseEnd() const
{
    return m_Bases.empty() ? 0 : std::prev(m_Bases.end())->first + 1;
}

uint64_t CentaurusBank::BankId() const
{
    return m_BankId;
}

const std::wstring& CentaurusBank::BankName() const
{
    return m_BankName;
}
