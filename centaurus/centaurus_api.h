#ifndef __CENTAURUS_API_H
#define __CENTAURUS_API_H

#ifdef CENTAURUS_INTERNAL
#include "centaurus.h"
#include <utility>
#include <vector>
#include <tuple>

#include <boost/atomic.hpp>
#include <boost/thread.hpp>

class CCentaurusAPI : public ICentaurusAPI
{
    using Task = std::tuple<std::unique_ptr<ICentaurusTask>, boost::thread>;
public:
    void Init() override;
    void Exit() override;

    void SetTableSizeLimit(centaurus_size limit) override;

    ICentaurusBank* ConnectBank(const std::wstring& path) override;
    void DisconnectBank(ICentaurusBank* bank) override;
    
    void ExportABIHeader(const CronosABI* abi, FILE* out) const override;
    void LogBankFiles(ICentaurusBank* bank) const override;
    void LogBuffer(const CroBuffer& buf, unsigned codepage = 0) override;

    void StartTask(ICentaurusTask* task) override;
    void EndTask(ICentaurusTask* task) override;

    Task* GetTask(ICentaurusTask* task);

    void TaskAwait() override;
    void TaskNotify(ICentaurusTask* task) override;
    void Idle(ICentaurusTask* task = NULL) override;

    bool IsBankAcquired(ICentaurusBank* bank) override;
    
    centaurus_size TotalMemoryUsage() override;
    centaurus_size RequestTableLimit() override;
private:
    FILE* m_fOutput;
    FILE* m_fError;

    centaurus_size m_TableSizeLimit;

    boost::mutex m_BankLock;
    std::vector<std::unique_ptr<ICentaurusBank>> m_Banks;

    boost::mutex m_TaskLock;
    boost::condition_variable m_TaskCond;
    boost::atomic<ICentaurusTask*> m_Notifier;
    boost::atomic<float> m_fNotifierProgress;
    std::vector<Task> m_Tasks;
};
#endif

#endif