#ifndef __CENTAURUS_API_H
#define __CENTAURUS_API_H

#include <stdint.h>
#include <functional>
#include <string>
#include <vector>
#include <map>

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

class CronosABI;
class CroBuffer;
class CroTable;
class CroEntry;
class CroFile;
class CroBank;
class CroAttr;
class CroBase;
class CroRecordMap;

class ICronosAPI;

#ifndef CROBANK_TYPES
#define CROBANK_TYPES
typedef enum : unsigned {
    CROFILE_STRU,
    CROFILE_BANK,
    CROFILE_INDEX,
    CROFILE_COUNT,
} crobank_file;
#endif

class ICentaurusBank
{
public:
    virtual ~ICentaurusBank() {}

    virtual CroBank* Bank() = 0;
    virtual CroFile* BankFile(crobank_file file) = 0;

    virtual bool Connect() = 0;
    virtual void Disconnect() = 0;

    virtual void Load(ICronosAPI* cro) = 0;

    virtual uint32_t BankFormSaveVer() const = 0;
    virtual uint32_t BankId() const = 0;
    virtual const std::wstring& BankName() const = 0;
    virtual uint32_t BankSerial() const = 0;
    virtual uint32_t BankCustomProt() const = 0;
    virtual const std::wstring& BankSysPass() const = 0;
    virtual int BankVersion() const = 0;

    virtual bool IsValidBase(unsigned index) const = 0;
    virtual CroBase& Base(unsigned index) = 0;
    virtual unsigned BaseEnd() const = 0;
};

/* CentaurusTask */

class ICentaurusWorker;

class ICentaurusTask
{
public:
    virtual ~ICentaurusTask() {}

    virtual void Invoke(ICentaurusWorker* invoker) = 0;
    virtual void RunTask() = 0;
    virtual void Release() = 0;
    virtual centaurus_size GetMemoryUsage() = 0;

    virtual bool AcquireBank(ICentaurusBank* bank) = 0;
    virtual CroTable* AcquireTable(CroTable&& table) = 0;
    virtual bool IsBankAcquired(ICentaurusBank* bank) = 0;
    virtual void ReleaseTable(CroTable* table) = 0;

    virtual ICentaurusWorker* Invoker() const = 0;
    virtual const std::string& TaskName() const = 0;
};

/* CentaurusLogger */

enum CentaurusLogLevel
{
    LogOutput,
    LogError
};

class ICentaurusLogger
{
public:
    virtual ~ICentaurusLogger() {}

    virtual void OpenLog(const std::wstring& outputLog,
        const std::wstring& errorLog) = 0;
    virtual void SetLogIO() = 0;
    virtual bool IsLogOpen() const = 0;
    virtual void CloseLog() = 0;

    virtual void LockLogger() = 0;
    virtual void UnlockLogger() = 0;
    virtual void* GetLogMutex() = 0;
    virtual const std::string& GetLogName() const = 0;
    virtual ICentaurusLogger* LogForwarder() = 0;

    virtual void LogPrint(CentaurusLogLevel lvl,
        const char* fmt, va_list ap) = 0;
    virtual void LogPrint(CentaurusLogLevel lvl,
        const char* fmt, ...) = 0;

    virtual void LogBankFiles(ICentaurusBank* bank) = 0;
    virtual void LogBankStructure(ICentaurusBank* bank) = 0;
    virtual void LogBuffer(const CroBuffer& buf, unsigned codepage = 0) = 0;
    virtual void LogTable(const CroTable& table) = 0;
    virtual void LogRecordMap(const CroRecordMap& records) = 0;

    virtual void Log(const char* fmt, ...) = 0;
    virtual void Error(const char* fmt, ...) = 0;
};

/* CentaurusWorker */

class ICentaurusWorker
{
public:
    enum state : unsigned {
        Waiting,
        Running,
        Terminated
    };

    virtual ~ICentaurusWorker() {}
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual void Wait() = 0;
    virtual state State() const = 0;

    virtual void SetWorkerLogger(ICentaurusLogger* log) = 0;
    virtual ICentaurusLogger* GetWorkerLogger() = 0;

    virtual void SetMemoryLimit(centaurus_size limit) = 0;
    virtual centaurus_size GetMemoryLimit() const = 0;
protected:
    virtual void Execute() = 0;
};

/* CronosAPI */

class ICronosAPI
{
public:
    virtual void LoadBank(ICentaurusBank* bank) = 0;
    virtual CroFile* SetLoaderFile(crobank_file ftype) = 0;
    virtual ICentaurusBank* TargetBank() const = 0;
    virtual CroBank* Bank() const = 0;

    virtual CroRecordMap* GetRecordMap(unsigned id, unsigned count) = 0;
    virtual CroBuffer GetRecord(unsigned id) = 0;
    virtual unsigned Start() const = 0;
    virtual unsigned End() const = 0;
    virtual void ReleaseMap() = 0;

    virtual ICentaurusLogger* CronosLog() = 0;
};

/* CentaurusExport */

enum ExportFormat {
    ExportCSV,
    ExportJSON
};

class ICentaurusExport
{
public:
    virtual const std::wstring& ExportPath() const = 0;
    virtual ExportFormat GetExportFormat() const = 0;
    virtual void SetExportFormat(ExportFormat fmt) = 0;
    virtual void SyncBankJson() = 0;
};

/* CentaurusAPI */

#define CENTAURUS_API_TABLE_LIMIT (512*1024*1024)
#define CENTAURUS_API_WORKER_LIMIT (4)

class ICentaurusAPI
{
public:
    virtual ~ICentaurusAPI() {}

    virtual void Init(const std::wstring& path) = 0;
    virtual void Exit() = 0;

    virtual void SetMemoryLimit(centaurus_size limit) = 0;
    virtual void SetWorkerLimit(unsigned threads) = 0;
    virtual centaurus_size TotalMemoryUsage() = 0;
    virtual centaurus_size GetMemoryLimit() = 0;
    virtual centaurus_size GetWorkerMemoryLimit() = 0;

    virtual void PrepareDataPath(const std::wstring& path) = 0;
    virtual std::wstring GetExportPath() const = 0;
    virtual std::wstring GetTaskPath() const = 0;
    virtual std::wstring GetBankPath() const = 0;

    virtual void StartWorker(ICentaurusWorker* worker) = 0;
    virtual void StopWorker(ICentaurusWorker* worker) = 0;

    virtual std::wstring BankFile(ICentaurusBank* bank) = 0;
    virtual void ConnectBank(const std::wstring& dir) = 0;
    virtual void DisconnectBank(ICentaurusBank* bank) = 0;
    virtual ICentaurusBank* FindBank(const std::wstring& path) = 0;
    virtual ICentaurusBank* WaitBank(const std::wstring& dir) = 0;

    virtual void ExportABIHeader(const CronosABI* abi, FILE* out = NULL) = 0;

    virtual bool IsBankLoaded(ICentaurusBank* bank) = 0;
    virtual bool IsBankAcquired(ICentaurusBank* bank) = 0;

    virtual std::wstring TaskFile(ICentaurusTask* task) = 0;
    virtual void StartTask(ICentaurusTask* task) = 0;
    virtual void ReleaseTask(ICentaurusTask* task) = 0;

    virtual void Run() = 0;
    virtual void Sync(ICentaurusWorker* worker) = 0;
    virtual void OnException(const std::exception& exc) = 0;
    virtual void OnWorkerException(ICentaurusWorker* worker,
        const std::exception& exc) = 0;

    virtual bool IsBankExported(uint32_t bankId) = 0;
    virtual void UpdateBankExportIndex(uint32_t bankId,
        const std::wstring& path) = 0;
};

/* Centaurus API */

extern CENTAURUS_API ICentaurusAPI* centaurus;

CENTAURUS_API void CentaurusAPI_RunThread();
CENTAURUS_API void CentaurusAPI_Idle();

#endif