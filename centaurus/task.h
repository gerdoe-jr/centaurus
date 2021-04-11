#ifndef __CENTAURUS_TASK_H
#define __CENTAURUS_TASK_H

#include "centaurus.h"
#include "logger.h"
#include <crotable.h>
#include <utility>
#include <vector>

#include <boost/atomic.hpp>
#include <boost/thread/mutex.hpp>

class CentaurusTask : public ICentaurusTask, protected CentaurusLogger
{
public:
    CentaurusTask();
    CentaurusTask(const std::string& taskName);
    virtual ~CentaurusTask();

    virtual void Invoke(ICentaurusWorker* invoker = NULL);
    virtual void RunTask();
    virtual void Release();
    virtual centaurus_size GetMemoryUsage();
    
    bool AcquireBank(ICentaurusBank* bank) override;
    CroTable* AcquireTable(CroTable&& table) override;
    bool IsBankAcquired(ICentaurusBank* bank) override;
    void ReleaseTable(CroTable* table) override;

    void AcquireTable(CroTable* table);
    template<typename T> inline T* AcquireTable(T&& table)
    {
        T* newTable = new T(std::move(table));
        AcquireTable(dynamic_cast<CroTable*>(newTable));
        return newTable;
    }

    ICentaurusWorker* Invoker() const override;
    const std::string& TaskName() const override;

    boost::atomic<float> m_fTaskProgress;
protected:
    boost::mutex m_DataLock;
    std::vector<ICentaurusBank*> m_Banks;
    std::vector<std::unique_ptr<CroTable>> m_Tables;
private:
    std::string m_TaskName;
    ICentaurusWorker* m_pInvoker;
};

#endif