#include "croexception.h"
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

const char* CroException::what() noexcept
{
    sprintf_s(s_szError, 256, "CroException - \"%s\"",
            File()->GetError().c_str());
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
