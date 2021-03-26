#ifndef __CENTAURUS_TASK_H
#define __CENTAURUS_TASK_H

#include "centaurus.h"
#include <crotable.h>
#include <utility>
#include <vector>

#include <boost/atomic.hpp>
#include <boost/thread/mutex.hpp>

class CentaurusTask : public ICentaurusTask
{
public:
    virtual ~CentaurusTask();

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

    boost::atomic<float> m_fTaskProgress;
protected:
    boost::mutex m_DataLock;
    std::vector<ICentaurusBank*> m_Banks;
    std::vector<std::unique_ptr<CroTable>> m_Tables;
};

#endif