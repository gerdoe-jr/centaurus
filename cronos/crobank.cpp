#include "crobank.h"
#include <stdexcept>

/* CroBank */

CroBank::CroBank(const std::wstring& path)
    : m_Path(path),
    m_TextCodePage(CRONOS_DEFAULT_CODEPAGE)
{
    m_Parser = NULL;
}

CroBank::~CroBank()
{
    Close();
}

bool CroBank::Open()
{
    for (unsigned i = 0; i < CROFILE_COUNT; i++)
    {
        std::wstring path = m_Path + L"\\" + FileName((crobank_file)i);
        std::unique_ptr<CroFile> file = std::make_unique<CroFile>(path);
        try {
            file->Open();
        }
        catch (const CroException& exc) {
            OnCronosException(exc);
            file->SetError(CROFILE_ERROR);
        }

        if (!file->IsFailed())
            m_CroFile[i] = std::move(file);
        else m_CroFile[i] = NULL;
    }

    return File(CROFILE_STRU) && File(CROFILE_BANK) && File(CROFILE_INDEX);
}

void CroBank::Close()
{
    for (unsigned i = 0; i < CROFILE_COUNT; i++)
    {
        auto& file = m_CroFile[i];
        if (file)
        {
            file->Close();
            file = NULL;
        }
    }
}

bool CroBank::TryBankPath()
{
    for (unsigned i = 0; i < CROFILE_COUNT; i++)
    {
        std::wstring path = m_Path + L"\\" + FileName((crobank_file)i);
        std::unique_ptr<CroFile> file = std::make_unique<CroFile>(path);
        try {
            if (file->Open() != CROFILE_OK)
                return false;
        }
        catch (const CroException& exc) {
            OnCronosException(exc);
            return false;
        }
    }

    return true;
}

void CroBank::SetBankPath(const std::wstring& path)
{
    m_Path = path;
}

const std::wstring& CroBank::GetBankPath() const
{
    return m_Path;
}

void CroBank::SetTextCodePage(unsigned cp)
{
    m_TextCodePage = cp;
}

unsigned CroBank::GetTextCodePage() const
{
    return m_TextCodePage;
}

std::wstring CroBank::GetWString(const uint8_t* str, cronos_size len)
{
    throw std::runtime_error("CroBank::GetWString not implemented");
}

std::string CroBank::GetString(const uint8_t* str, cronos_size len)
{
    return std::string((const char*)str, len);
}

CroFile* CroBank::File(crobank_file file)
{
    return m_CroFile[file].get();
}

const std::wstring& CroBank::FileName(crobank_file file)
{
    static std::wstring _croFileNames[] = {
        L"CroStru",
        L"CroBank",
        L"CroIndex",
    };

    return _croFileNames[file];
}

CroParser* CroBank::Parser()
{
    return m_Parser;
}

void CroBank::ParserStart(CroParser* parser)
{
    m_Parser = parser;
}

void CroBank::ParserEnd(CroParser* parser)
{
    m_Parser = NULL;
}

void CroBank::OnCronosException(const CroException& exc)
{
    throw std::runtime_error("bank cronos exception not implemented");
}

void CroBank::OnParseProp(CroProp& prop)
{

}
