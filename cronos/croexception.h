#ifndef __CROEXCEPTION_H
#define __CROEXCEPTION_H

#include "crotype.h"
#include <exception>
#include <string>

class CroFile;
class CronosABI;

class CroException : public std::exception
{
public:
    CroException(CroFile* file);
    CroException(CroFile* file, const std::string& error);
    CroException(CroFile* file, const std::string& func, cronos_id id);
    CroException(CroFile* file, const std::string& func, cronos_off off);

    inline CroFile* File() const { return m_pFile; }
    virtual const char* what() noexcept;
private:
    CroFile* m_pFile;
};

class CroStdError : public CroException
{
public:
    CroStdError(CroFile* file);

    const char* what() noexcept override;
};

class CroABIError : public CroException
{
public:
    CroABIError(const CronosABI* abi, unsigned value,
        const std::string& valueName);

    const char* what() noexcept override;
private:
    const CronosABI* m_pABI;
    unsigned m_Value;
    const std::string& m_ValueName;
};

#endif
