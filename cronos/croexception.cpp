#include "croexception.h"
#include "cronos_abi.h"
#include "crofile.h"
#include <stdio.h>
#include <errno.h>

#ifndef WIN32
#define sprintf_s snprintf
#endif

static std::string s_Error;

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

const char* CroException::what() const throw()
{
    s_Error = "CroException - \"" + (File()
        ? File()->GetError() : "CroException") + "\"";
    return s_Error.c_str();
}

CroStdError::CroStdError(CroFile* file)
    : CroException(file)
{
}

const char* CroStdError::what() const throw()
{
    s_Error = "CroStdError - \"" + File()->GetError()
        + "\", errno = " + std::to_string(errno);
    return s_Error.c_str();
}

CroABIError::CroABIError(const CronosABI* abi, unsigned value,
    const std::string& valueName)
    : CroException(NULL),
    m_pABI(abi), m_Value(value),
    m_ValueName(valueName)
{
}

const char* CroABIError::what() const throw()
{
    cronos_abi_num ver = m_pABI->GetABIVersion();
    const auto* value = m_pABI->GetValue((cronos_value)m_Value);

    char szError[256] = {0};
    sprintf_s(szError, sizeof(szError),
        "Cronos %dX [%02d.%02d] Error\n"
        "%s\n"
        "\tcronos_off\t%" FCroOff "\n"
        "\tcronos_size\t%" FCroSize,

        m_pABI->GetVersion(), ver.first, ver.second,
        m_ValueName.c_str(),
        value->m_Offset, value->m_Size
    );

    s_Error = szError;
    return s_Error.c_str();
}