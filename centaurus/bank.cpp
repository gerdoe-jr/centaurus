#include "bank.h"
#include "export.h"
#include <crostru.h>
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

/* CentaurusBank */

CentaurusBank::CentaurusBank()
    : CroBank(L"")
{
    m_BankId = 0;
}

CentaurusBank::~CentaurusBank()
{
    Disconnect();
}

std::wstring CentaurusBank::GetWString(const uint8_t* str, cronos_size len)
{
    return AnsiToWchar(std::string((const char*)str, len), m_TextCodePage);
}

std::string CentaurusBank::GetString(const uint8_t* str, cronos_size len)
{
    return WcharToText(GetWString(str, len));
}

void CentaurusBank::BankCronosException(const CroException& exc)
{
    centaurus->OnException(exc);
}

CroBank* CentaurusBank::Bank()
{
    return dynamic_cast<CroBank*>(this);
}

CroFile* CentaurusBank::BankFile(crobank_file file)
{
    return File(file);
}

bool CentaurusBank::Connect()
{
    return Bank()->Open();
}

void CentaurusBank::Disconnect()
{
    Bank()->Close();
}

/*void CentaurusBank::LoadStructure(ICentaurusExport* exp)
{
    CroFile* stru = File(CroStru);
    if (!stru) throw std::runtime_error("no structure");
    auto* abi = stru->ABI();
    
    exp->ReadRecord(stru, 1, m_BankRecord);
    m_AttrStream = CroStream(m_BankRecord);
    
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
    printf("NS1 BankSysPass %s\n", ns1.BankSysPassword().c_str());
    centaurus->LogBuffer(ns1Attr.GetAttr());
}*/

void CentaurusBank::LoadStructure(ICronosAPI* cro)
{
    auto* log = cro->CronosLog();
    
    CroFile* file = cro->SetLoaderFile(CROFILE_STRU);
    
    if (!file) throw std::runtime_error("no structure");
    uint32_t key = file->GetSecretKey(file->GetSecret());
    file->SetupCrypt(key, CRONOS_DEFAULT_SERIAL);

    CroStru stru = CroStru(this, cro->GetRecordMap(
        1, file->EntryCountFileSize()));
    
    CroBuffer attr;
    CroStream props;
    if (stru.GetAttrByName("Bank", attr, props))
    {
        log->LogBuffer(attr, 1251);
    }
    
    cro->ReleaseMap();
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
