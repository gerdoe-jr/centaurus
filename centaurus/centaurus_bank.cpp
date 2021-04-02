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
    m_BankId = 0;
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

bool CentaurusBank::Connect()
{
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
                fprintf(stderr, "CroFile status %u\n", st);
                m_Files[(CroBankFile)i] = NULL;
                continue;
            }

            cronos_size limit = centaurus->RequestTableLimit();
            file->SetTableLimits(limit);
        }
        catch (const std::exception& e) {
            centaurus->OnException(e);
            fprintf(stderr, "CroFile(%s) %s\n", WcharToAnsi(
                file->GetPath()).c_str(), e.what());
            m_Files[(CroBankFile)i] = NULL;
        }
    }

    return File(CroStru) && File(CroBank);
}

void CentaurusBank::Disconnect()
{
    for (unsigned i = 0; i < CroBankFile_Count; i++)
    {
        auto& file = m_Files[i];
        if (file)
        {
            file->Close();
            file = NULL;
        }
    }
}

void CentaurusBank::AssociatePath(const std::wstring& dir)
{
    m_Path = dir;
}

std::wstring CentaurusBank::GetPath() const
{
    return m_Path;
}

void CentaurusBank::SetCodePage(unsigned codepage)
{
    m_uCodePage = codepage;
}

CroFile* CentaurusBank::File(CroBankFile type) const
{
    return !m_Files[type] ? NULL : m_Files[type].get();
}

std::string CentaurusBank::String(const char* data, size_t len)
{
    return WcharToText(AnsiToWchar(std::string(data, len), m_uCodePage));
}

CroFile* CentaurusBank::CroFileStru()
{
    return File(CroBankFile::CroStru);
}

CroFile* CentaurusBank::CroFileBank()
{
    return File(CroBankFile::CroBank);
}

CroFile* CentaurusBank::CroFileIndex()
{
    return File(CroBankFile::CroIndex);
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

CroAttr CentaurusBank::LoadAttribute(ICentaurusExport* exp)
{
    CroAttr attr;
    attr.Parse(this, m_AttrStream);

    CroBuffer data;
    CroStream stream(data);

    if (attr.IsRef())
    {
        exp->ReadRecord(File(CroStru), attr.RefBlockId(), data);
        
        uint8_t prefix = stream.Read<uint8_t>();
        if (prefix != CROATTR_REF_PREFIX)
        {
            throw std::runtime_error(
                "ref attribute prefix != CROBASE_PREFIX");
        }
    }
    else
    {
        cronos_size size = attr.AttrSize();
        data.Write(m_AttrStream.Read(size), size);
    }

    cronos_size available = stream.Remaining();
    attr.GetAttr().Copy(stream.Read(available), available);
    return attr;
}

void CentaurusBank::LoadStructure(ICentaurusExport* exp)
{
    CroFile* stru = File(CroStru);
    if (!stru) throw std::runtime_error("no structure");

    exp->ReadRecord(stru, 1, m_BankRecord);
    m_AttrStream = CroStream(m_BankRecord);
    
    auto* abi = stru->ABI();
    if (abi->IsLite())
    {
        CroData secret = stru->Read(0, 1, cronos_hdr_secret);
        CroData lite = stru->Read(0, 1, cronos_hdrlite_secret);
        centaurus->LogBuffer(secret);
        centaurus->LogBuffer(lite);
    }

    auto& stream = m_AttrStream;
    uint8_t prefix = stream.Read<uint8_t>();
    if (prefix != CROATTR_PREFIX)
    {
        printf("prefix %u\n", prefix);
        centaurus->LogBuffer(m_BankRecord);
        throw std::runtime_error("not an attr prefix");
    }

    CroAttr attr;
    try {
        while (!stream.IsOverflowed())
        {
            CroAttr attr = LoadAttribute(exp);
            
            m_Attrs.push_back(attr);
        }
    }
    catch (const std::exception& e) {
        fprintf(stderr, "[CentaurusBank] CroAttr(%s) exception\n",
            attr.GetName().c_str());
#ifdef CENTAURUS_DEBUG
        CroBuffer& _attr = attr.GetAttr();
        if (!_attr.IsEmpty()) centaurus->LogBuffer(_attr);
#endif
        centaurus->OnException(e);
    }

    m_BankId = (uint32_t)atoi(Attr("BankId").String());
    m_BankName = AnsiToWchar(Attr("BankName").GetString());

    CroAttrNS ns1;
    CroAttr& ns1Attr = Attr("NS1");
    ns1.Parse(this, ns1Attr);
    
    printf("NS1 BankSerial %u\n", ns1.BankSerial());
    printf("NS1 BankCustomProt %u\n", ns1.BankCustomProt());
    printf("NS1 BankUnk %u\n", ns1.BankUnk());
    centaurus->LogBuffer(ns1Attr.GetAttr());
}

void CentaurusBank::LoadBases(ICentaurusExport* exp)
{
    for (auto& attr : m_Attrs)
    {
        std::string attrName = attr.GetName();
        if (!attrName.starts_with("Base") || attrName.size() != 7)
            continue;

        try {
            CroBase base;
            
            cronos_idx index = base.Parse(this, attr);
            m_Bases.insert(std::make_pair(index, base));
        }
        catch (const std::exception& e) {
            fprintf(stderr, "[CentaurusBank] CroBase(%s) exception\n",
                attr.GetName().c_str());
#ifdef CENTAURUS_DEBUG
            if (attr.IsRef())
            {
                fprintf(stderr, "\tRefId %" FCroId "\n", attr.RefBlockId());
            }
            else centaurus->LogBuffer(attr.GetAttr());
#endif
            centaurus->OnException(e);
        }
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

uint32_t CentaurusBank::BankId() const
{
    return m_BankId;
}

const std::wstring& CentaurusBank::BankName() const
{
    return m_BankName;
}
