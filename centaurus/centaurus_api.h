#ifndef __CENTAURUS_API_H
#define __CENTAURUS_API_H

#ifdef CENTAURUS_INTERNAL
#include "centaurus.h"
#include "centaurus_worker.h"
#include <utility>
#include <vector>
#include <queue>
#include <tuple>

#include <boost/atomic.hpp>
#include <boost/thread.hpp>

class CentaurusJob : public CentaurusWorker
{
public:
    CentaurusJob(ICentaurusTask* task);
    virtual ~CentaurusJob();

    void Execute() override;
    
    ICentaurusTask* JobTask();
protected:
    std::unique_ptr<ICentaurusTask> m_pTask;
};

class CentaurusLoader : public CentaurusWorker
{
public:
    void RequestBank(const std::wstring& path);
    void RequestBanks(std::vector<std::wstring> dirs);
protected:
    void Execute() override;

    bool LoadPath(const std::wstring& path);
    void LogLoaderFail(const std::wstring& dir);
private:
    std::queue<std::wstring> m_Dirs;
public:
    std::wstring m_LoadedPath;
    ICentaurusBank* m_pLoadedBank;
};

class CentaurusAPI : public ICentaurusAPI
{
public:
    void Init(const std::wstring& path) override;
    void Exit() override;

    void SetTableSizeLimit(centaurus_size limit) override;
    
    void PrepareDataPath(const std::wstring& path) override;
    std::wstring GetExportPath() const override;
    std::wstring GetTaskPath() const override;
    std::wstring GetBankPath() const override;

    std::wstring BankFile(ICentaurusBank* bank) override;
    void ConnectBank(const std::wstring& path) override;
    void DisconnectBank(ICentaurusBank* bank) override;
    ICentaurusBank* FindBank(const std::wstring& path) override;
    ICentaurusBank* WaitBank(const std::wstring& path) override;
    
    void ExportABIHeader(const CronosABI* abi, FILE* out) const override;
    void LogBankFiles(ICentaurusBank* bank) const override;
    void LogBuffer(const CroBuffer& buf, unsigned codepage = 0) override;
    void LogTable(const CroTable& table) override;

    bool IsBankLoaded(ICentaurusBank* bank) override;
    bool IsBankAcquired(ICentaurusBank* bank) override;
    
    centaurus_size TotalMemoryUsage() override;
    centaurus_size RequestTableLimit() override;

    std::wstring TaskFile(ICentaurusTask* task) override;
    void StartTask(ICentaurusTask* task) override;
    
    void Run() override;
    void Sync(ICentaurusWorker* worker) override;
private:
    FILE* m_fOutput;
    FILE* m_fError;

    centaurus_size m_TableSizeLimit;
    std::wstring m_DataPath;

    boost::mutex m_BankLock;
    std::vector<std::unique_ptr<ICentaurusBank>> m_Banks;
    std::unique_ptr<CentaurusLoader> m_pLoader;

    boost::mutex m_TaskLock;
    std::vector<std::unique_ptr<CentaurusJob>> m_Tasks;

    boost::mutex m_SyncLock;
    boost::condition_variable m_SyncCond;
};
#endif

#endif