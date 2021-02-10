#include "croexception.h"
#include "cronos_abi.h"
#include "crofile.h"
#include <stdio.h>
#include <errno.h>

static char s_szError[256] = {0};

CroException::CroException(CroFile* file)
    : m_pFile(file)
{
}

CroException::CroException(CroFile* file, const std::string& error)
    : m_pFile(file)
{
    m_pFile->SetError(error);
}

CroException::CroException(CroFile* file, const std::string& func,
    cronos_off off)
    : m_pFile(file)
{
    m_pFile->SetError(func + "invalid cronos_off("
        + std::to_string(off) + ")");
}

CroException::CroException(CroFile* file, const std::string& func,
    cronos_id id)
    : m_pFile(file)
{
    m_pFile->SetError(func + "invalid (entry) id("
        + std::to_string(id) + ")");
}

const char* CroException::what() noexcept
{
    sprintf_s(s_szError, 256, "CroException - \"%s\"",
        File() ? File()->GetError().c_str() : "CroException"
    );
    return s_szError;
}

CroStdError::CroStdError(CroFile* file)
    : CroException(file)
{
}

const char* CroStdError::what() noexcept
{
    sprintf_s(s_szError, 256, "CroStdError - \"%s\", errno = %d",
            File()->GetError().c_str(), errno);
    return s_szError;
}

CroABIError::CroABIError(const CronosABI* abi, unsigned value,
    const std::string& valueName)
    : CroException(NULL),
    m_pABI(abi), m_Value(value),
    m_ValueName(valueName)
{
}

const char* CroABIError::what() noexcept
{
    cronos_abi_num ver = m_pABI->GetABIVersion();
    const auto* value = m_pABI->GetValue((cronos_value)m_Value);

    sprintf_s(s_szError, 256,
        "Cronos %dX [%02d.%02d] Error\n"
        "%s\n"
        "\tcronos_off\t%" FCroOff "\n"
        "\tcronos_size\t%" FCroSize,

        m_pABI->GetVersion(), ver.first, ver.second,
        m_ValueName.c_str(),
        value->m_Offset, value->m_Size
    );
    return s_szError;
}