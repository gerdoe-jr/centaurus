#include "centaurus_bank.h"
#include "centaurus_export.h"
#include "croattr.h"
#include <stdexcept>
#include <fstream>

#include <boost/json.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
using boost::property_tree::ptree;
namespace pt = boost::property_tree;
namespace fs = boost::filesystem;
namespace sc = boost::system;

CentaurusBank::CentaurusBank()
{
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

void CentaurusBank::LoadBase(ICentaurusExport* exp, CroAttr& attr)
{
    CroBuffer base;
    if (attr.IsEntryId())
    {
        uint32_t id = *(uint32_t*)attr.GetAttr().GetData();
        exp->ReadRecord(File(CroStru), id, base);
    }
    else base = CroBuffer(attr.GetAttr());

    m_Bases.push_back(base);
}

void CentaurusBank::ParseAttr(ICentaurusExport* exp,
    CroStream& stream, CroAttr& attr)
{
    attr.Parse(stream);
    std::string name = attr.GetName();

    if (name.starts_with("Base") && name.length() == 7)
        LoadBase(exp, attr);
}

void CentaurusBank::LoadStructure(ICentaurusExport* exp)
{
    CroFile* stru = File(CroStru);
    if (!stru) throw std::runtime_error("no structure");

    CroBuffer bankStruct;
    exp->ReadRecord(stru, 1, bankStruct);
    
    CroStream stream = CroStream(bankStruct);
    stream.Read<uint8_t>(); // record prefix

    while (!stream.IsOverflowed())
    {
        CroAttr attr;
        ParseAttr(exp, stream, attr);

        m_Attrs.push_back(attr);
    }
}

void CentaurusBank::ExportStructure(ICentaurusExport* exp)
{
    ptree bank;
    for (auto& attr : m_Attrs)
        bank.put(attr.GetName(), attr.GetString());

    std::ofstream json = std::ofstream(exp->ExportPath() + L"\\bank.json");
    pt::write_json(json, bank);
    json.close();
}
