#ifndef __CENTAURUS_H
#define __CENTAURUS_H

#include "centaurus_api.h"
#include <stdint.h>
#include <string>

typedef uint64_t centaurus_size;

enum CroBankFile {
    CroStru,
    CroBank,
    CroIndex,
    CroBankFile_Count
};

class CronosABI;
class CroBuffer;
class CroFile;

class ICentaurusBank
{
public:
    virtual ~ICentaurusBank() {}

    virtual bool LoadPath(const std::wstring& dir) = 0;
    virtual CroFile* File(CroBankFile file) const = 0;

    virtual void ExportHeaders() const = 0;
};

class ICentaurusTask
{
public:
    virtual ~ICentaurusTask() {}

    virtual void StartTask() = 0;
    virtual void EndTask() = 0;
    virtual void Interrupt() = 0;

    virtual void Run() = 0;

    virtual float GetTaskProgress() const = 0;
};

class ICentaurusAPI
{
public:
    virtual ~ICentaurusAPI() {}

    virtual void Init() = 0;
    virtual void Exit() = 0;

    virtual void SetTableSizeLimit(centaurus_size limit) = 0;

    virtual ICentaurusBank* ConnectBank(const std::wstring& dir) = 0;
    virtual void DisconnectBank(ICentaurusBank* bank) = 0;

    virtual void ExportABIHeader(const CronosABI* abi,
        FILE* out = NULL) const = 0;
    
    virtual void LogBankFiles(ICentaurusBank* bank) const = 0;
    virtual void LogBuffer(const CroBuffer& buf, unsigned codepage = 0) = 0;

    virtual void StartTask(ICentaurusTask* task) = 0;
    virtual void EndTask(ICentaurusTask* task) = 0;
    virtual void TaskAwait() = 0;
    virtual void TaskNotify(ICentaurusTask* task) = 0;
    virtual void Idle(ICentaurusTask* task = NULL) = 0;
};

extern CENTAURUS_API ICentaurusAPI* centaurus;

CENTAURUS_API bool InitCentaurusAPI();
CENTAURUS_API void ExitCentaurusAPI();

#endif