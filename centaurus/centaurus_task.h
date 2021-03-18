#ifndef __CENTAURUS_TASK_H
#define __CENTAURUS_TASK_H

#ifdef CENTAURUS_INTERNAL
#include "centaurus.h"
#include <crotable.h>
#include <utility>
#include <vector>

#include <boost/atomic.hpp>
#include <boost/thread/mutex.hpp>

class CentaurusTask : public ICentaurusTask
{
public:
    CentaurusTask();
    CentaurusTask(CentaurusRun run);
    virtual ~CentaurusTask();

    virtual void StartTask();
    virtual void EndTask();
    virtual void Interrupt();
    virtual void Run();

    float GetTaskProgress() const override;
    void UpdateProgress(float progress);

    bool AcquireBank(ICentaurusBank* bank) override;

    void AcquireTable(CroTable* table);
    CroTable* AcquireTable(CroTable&& table) override;
    template<typename T> inline T* AcquireTable(T&& table)
    {
        T* newTable = new T(std::move(table));
        AcquireTable(dynamic_cast<CroTable*>(newTable));
        return newTable;
    }

    bool IsBankAcquired(ICentaurusBank* bank) override;
    void ReleaseTable(CroTable* table) override;
    centaurus_size GetMemoryUsage() override;
private:
    boost::atomic<float> m_fTaskProgress;
    CentaurusRun m_RunFunction;

    boost::mutex m_DataLock;
    std::vector<ICentaurusBank*> m_Banks;
    std::vector<std::unique_ptr<CroTable>> m_Tables;
};
#endif

#endif