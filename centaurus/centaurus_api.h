#ifndef __CENTAURUS_API_H
#define __CENTAURUS_API_H

#include "centaurus.h"
#include "centaurus_loader.h"
#include "centaurus_scheduler.h"
#include <utility>
#include <vector>
#include <queue>
#include <tuple>

#include <boost/atomic.hpp>
#include <boost/thread.hpp>

#include <json_file.h>

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
    void LogBankFiles(ICentaurusBank* bank) override;
    void LogBuffer(const CroBuffer& buf, unsigned codepage = 0) override;
    void LogTable(const CroTable& table) override;

    bool IsBankLoaded(ICentaurusBank* bank) override;
    bool IsBankAcquired(ICentaurusBank* bank) override;
    
    centaurus_size TotalMemoryUsage() override;
    centaurus_size RequestTableLimit() override;

    std::wstring TaskFile(ICentaurusTask* task) override;
    void TaskSyncJSON(ICentaurusTask* task, json value);
    void StartTask(ICentaurusTask* task) override;
    void ReleaseTask(ICentaurusTask* task);
    
    void Run() override;
    void Sync(ICentaurusWorker* worker) override;
    void OnException(const std::exception& exc) override;
    void OnWorkerException(ICentaurusWorker* worker,
        const std::exception& exc) override;
private:
    boost::mutex m_LogLock;
    FILE* m_fOutput;
    FILE* m_fError;

    centaurus_size m_TableSizeLimit;
    std::wstring m_DataPath;

    boost::mutex m_BankLock;
    std::vector<std::unique_ptr<ICentaurusBank>> m_Banks;
    std::unique_ptr<CentaurusLoader> m_pLoader;

    boost::mutex m_TaskLock;
    std::vector<std::unique_ptr<ICentaurusTask>> m_Tasks;
    std::unique_ptr<CentaurusScheduler> m_pScheduler;

    boost::mutex m_SyncLock;
    boost::condition_variable m_SyncCond;

    std::vector<uint64_t> m_KnownBanks;
};

extern CentaurusAPI* _centaurus;

#endif