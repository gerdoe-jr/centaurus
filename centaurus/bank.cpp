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
    m_pCronos = NULL;
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

void CentaurusBank::OnCronosException(const CroException& exc)
{
    centaurus->OnException(exc);
}

void CentaurusBank::OnParseProp(CroProp& prop)
{
    CroBank::OnParseProp(prop);
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

void CentaurusBank::Load(ICronosAPI* cro)
{
    m_pCronos = cro;

    LoadStructure();

    m_pCronos = NULL;
}

void CentaurusBank::LoadStructure()
{
    CroFile* file = m_pCronos->SetLoaderFile(CROFILE_STRU);
    if (!file)
    {
        throw std::runtime_error("no structure");
    }
    
    uint32_t key = file->GetSecretKey(file->GetSecret());
    file->SetupCrypt(key, CRONOS_DEFAULT_SERIAL);

    auto map = file->LoadRecordMap(1, file->EntryCountFileSize());
    auto stru = CroStru(Bank(), &map);

    if (!stru.LoadBankProps())
    {
        throw std::runtime_error("failed to load bank props");
    }
}

uint32_t CentaurusBank::BankFormSaveVer() const
{
    return m_BankFormSaveVer;
}

uint32_t CentaurusBank::BankId() const
{
    return m_BankId;
}

const std::wstring& CentaurusBank::BankName() const
{
    return m_BankName;
}

uint32_t CentaurusBank::BankSerial() const
{
    return m_BankSerial;
}

uint32_t CentaurusBank::BankCustomProt() const
{
    return m_BankCustomProt;
}

const std::wstring& CentaurusBank::BankSysPass() const
{
    return m_BankSysPass;
}

int CentaurusBank::BankVersion() const
{
    return m_BankVersion;
}

bool CentaurusBank::IsValidBase(unsigned index) const
{
    return index < m_Bases.size();
}

CroBase& CentaurusBank::Base(unsigned index)
{
    /*auto it = m_Bases.find(index);
    if (it == m_Bases.end())
        throw std::runtime_error("invalid base");;
    return it->second;*/
    return m_Bases.at(index);
}

unsigned CentaurusBank::BaseEnd() const
{
    return m_Bases.empty() ? 0 : std::prev(m_Bases.end())->m_BaseIndex;
}
