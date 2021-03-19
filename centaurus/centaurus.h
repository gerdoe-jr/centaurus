#ifndef __CENTAURUS_H
#define __CENTAURUS_H

#include <stdint.h>
#include <string>
#include <functional>

#ifdef WIN32
#ifdef CENTAURUS_INTERNAL
#define CENTAURUS_API __declspec(dllexport)
#else
#define CENTAURUS_API __declspec(dllimport)
#endif
#else
#define CENTAURUS_API
#endif

typedef uint64_t centaurus_size;

/* CentaurusBank */

enum CroBankFile {
    CroStru,
    CroBank,
    CroIndex,
    CroBankFile_Count
};

class CronosABI;
class CroBuffer;
class CroTable;
class CroFile;
class CroAttr;
class CroBase;

class ICentaurusExport;

class ICentaurusBank
{
public:
    virtual ~ICentaurusBank() {}

    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;
    
    virtual void AssociatePath(const std::wstring& dir) = 0;
    virtual std::wstring GetPath() const = 0;
    virtual void SetCodePage(unsigned codepage) = 0;

    virtual CroFile* File(CroBankFile file) const = 0;
    virtual std::string String(const char* data, size_t len) = 0;

    virtual void ExportHeaders() const = 0;

    virtual void LoadStructure(ICentaurusExport* exp) = 0;
    virtual void LoadBases(ICentaurusExport* exp) = 0;

    virtual CroAttr& Attr(const std::string& name) = 0;
    virtual CroAttr& Attr(unsigned index) = 0;
    virtual unsigned AttrCount() const = 0;

    virtual bool IsValidBase(unsigned index) const = 0;
    virtual CroBase& Base(unsigned index) = 0;
    virtual unsigned BaseEnd() const = 0;

    virtual uint64_t BankId() const = 0;
    virtual const std::wstring& BankName() const = 0;
};

/* CentaurusTask */

class ICentaurusTask
{
public:
    virtual ~ICentaurusTask() {}

    virtual void StartTask() = 0;
    virtual void EndTask() = 0;
    virtual void Interrupt() = 0;

    virtual void Run() = 0;

    virtual float GetTaskProgress() const = 0;

    virtual bool AcquireBank(ICentaurusBank* bank) = 0;
    virtual CroTable* AcquireTable(CroTable&& table) = 0;
    virtual bool IsBankAcquired(ICentaurusBank* bank) = 0;
    virtual void ReleaseTable(CroTable* table) = 0;

    virtual void Release() = 0;

    virtual centaurus_size GetMemoryUsage() = 0;
};
using CentaurusRun = std::function<void(ICentaurusTask*)>;

/* CentaurusExport */

enum ExportFormat {
    ExportCSV,
    ExportJSON
};

class ICentaurusExport
{
public:
    virtual ICentaurusBank* TargetBank() = 0;
    virtual const std::wstring& ExportPath() const = 0;
    virtual ExportFormat GetExportFormat() const = 0;
    virtual void SetExportFormat(ExportFormat fmt) = 0;
    virtual void ReadRecord(CroFile* file, uint32_t id, CroBuffer& out) = 0;
    virtual void SyncBankJson() = 0;
};

/* CentaurusAPI */

class ICentaurusAPI
{
public:
    virtual ~ICentaurusAPI() {}

    virtual void Init() = 0;
    virtual void Exit() = 0;

    virtual void SetTableSizeLimit(centaurus_size limit) = 0;

    virtual void PrepareDataPath(const std::wstring& path) = 0;
    virtual std::wstring GetExportPath() const = 0;
    virtual std::wstring GetTaskPath() const = 0;
    virtual std::wstring GetBankPath() const = 0;

    virtual std::wstring BankFile(ICentaurusBank* bank) = 0;
    virtual ICentaurusBank* ConnectBank(const std::wstring& dir) = 0;
    virtual void DisconnectBank(ICentaurusBank* bank) = 0;
    virtual void WaitBank() = 0;

    virtual void ExportABIHeader(const CronosABI* abi,
        FILE* out = NULL) const = 0;

    virtual void LogBankFiles(ICentaurusBank* bank) const = 0;
    virtual void LogBuffer(const CroBuffer& buf, unsigned codepage = 0) = 0;
    virtual void LogTable(const CroTable& table) = 0;

    virtual std::wstring TaskFile(ICentaurusTask* task) = 0;
    virtual void StartTask(ICentaurusTask* task) = 0;
    virtual void EndTask(ICentaurusTask* task) = 0;
    virtual void TaskAwait() = 0;
    virtual void TaskNotify(ICentaurusTask* task) = 0;
    virtual void Idle(ICentaurusTask* task = NULL) = 0;

    virtual bool IsBankLoaded(ICentaurusBank* bank) = 0;
    virtual bool IsBankAcquired(ICentaurusBank* bank) = 0;

    virtual centaurus_size TotalMemoryUsage() = 0;
    virtual centaurus_size RequestTableLimit() = 0;
};

/* Centaurus */

extern CENTAURUS_API ICentaurusAPI* centaurus;

CENTAURUS_API bool Centaurus_Init();
CENTAURUS_API void Centaurus_Exit();

CENTAURUS_API ICentaurusTask* CentaurusTask_Run(CentaurusRun run);
CENTAURUS_API ICentaurusTask* CentaurusTask_Export(ICentaurusBank* bank);

#endif