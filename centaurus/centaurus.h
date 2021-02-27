#ifndef __CENTAURUS_H
#define __CENTAURUS_H

#include "centaurus_api.h"
#include <string>

enum CroBankFile {
    CroStru,
    CroBank,
    CroIndex,
    CroBankFile_Count
};

class CronosABI;
class CroFile;

class ICentaurusBank
{
public:
    virtual bool LoadPath(const std::wstring& dir) = 0;
    virtual CroFile* File(CroBankFile file) const = 0;

    virtual void ExportHeaders() const = 0;
};

class ICentaurusAPI
{
public:
    virtual ~ICentaurusAPI() {}

    virtual void Init() = 0;
    virtual void Exit() = 0;

    virtual ICentaurusBank* ConnectBank(const std::wstring& dir) = 0;
    virtual void DisconnectBank(ICentaurusBank* bank) = 0;

    virtual void ExportABIHeader(const CronosABI* abi,
        FILE* out = NULL) const = 0;
    virtual void LogBankFiles(ICentaurusBank* bank) const = 0;
};

extern CENTAURUS_API ICentaurusAPI* centaurus;

CENTAURUS_API bool InitCentaurusAPI();
CENTAURUS_API void ExitCentaurusAPI();

#endif