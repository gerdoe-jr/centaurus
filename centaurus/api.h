#ifndef __CENTAURUS_API_H
#define __CENTAURUS_API_H

#include "centaurus.h"
#include "scheduler.h"
#include "logger.h"
#include "fetch.h"
#include <utility>
#include <vector>
#include <queue>
#include <tuple>

#include <boost/atomic.hpp>
#include <boost/thread.hpp>

#include <json_file.h>

class CentaurusAPI : public ICentaurusAPI, public CentaurusLogger
{
public:
    CentaurusAPI();
    ~CentaurusAPI();

    void Init(const std::wstring& path) override;
    void Exit() override;

    void SetTableSizeLimit(centaurus_size limit) override;
    void SetWorkerLimit(unsigned count) override;
    
    void PrepareDataPath(const std::wstring& path) override;
    std::wstring GetExportPath() const override;
    std::wstring GetTaskPath() const override;
    std::wstring GetBankPath() const override;

    void StartWorker(ICentaurusWorker* worker) override;
    void StopWorker(ICentaurusWorker* worker) override;
    template<typename T> inline std::unique_ptr<T> CreateWorker()
    {
        T* worker = new T;
        StartWorker(worker);
        return std::unique_ptr<T>(worker);
    }

    std::wstring BankFile(ICentaurusBank* bank) override;
    void ConnectBank(const std::wstring& path) override;
    void DisconnectBank(ICentaurusBank* bank) override;
    ICentaurusBank* FindBank(const std::wstring& path) override;
    ICentaurusBank* WaitBank(const std::wstring& path) override;
    
    void ExportABIHeader(const CronosABI* abi, FILE* out) override;

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

    std::string SizeToString(centaurus_size size) const;

    bool IsBankExported(uint32_t bankId) override;
    void UpdateBankExportIndex(uint32_t bankId,
        const std::wstring& path) override;
private:
    centaurus_size m_TableSizeLimit;
    unsigned m_uWorkerLimit;
    std::wstring m_DataPath;

    boost::mutex m_BankLock;
    std::vector<std::unique_ptr<ICentaurusBank>> m_Banks;
    std::unique_ptr<CentaurusFetch> m_pFetch;

    boost::mutex m_TaskLock;
    std::vector<std::unique_ptr<ICentaurusTask>> m_Tasks;
    std::unique_ptr<CentaurusScheduler> m_pScheduler;

    boost::mutex m_SyncLock;
    boost::condition_variable m_SyncCond;

    std::vector<uint64_t> m_KnownBanks;
    std::vector<std::wstring> m_FailedBanks;

    boost::mutex m_ExportLock;
    json m_ExportIndex;
};

extern CentaurusAPI* _centaurus;

#endif