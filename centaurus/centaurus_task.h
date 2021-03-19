#ifndef __CENTAURUS_TASK_H
#define __CENTAURUS_TASK_H

#ifdef CENTAURUS_INTERNAL
#include "centaurus.h"
#include <crotable.h>
#include <utility>
#include <vector>

#include <boost/atomic.hpp>
#include <boost/thread/mutex.hpp>

//class CentaurusTask : public ICentaurusTask
//{
//public:
//    CentaurusTask();
//    CentaurusTask(CentaurusRun run);
//    virtual ~CentaurusTask();
//
//    virtual void StartTask();
//    virtual void EndTask();
//    virtual void Interrupt();
//    virtual void Run();
//
//    float GetTaskProgress() const override;
//    void UpdateProgress(float progress);
//
//    bool AcquireBank(ICentaurusBank* bank) override;
//
//    void AcquireTable(CroTable* table);
//    CroTable* AcquireTable(CroTable&& table) override;
//    template<typename T> inline T* AcquireTable(T&& table)
//    {
//        T* newTable = new T(std::move(table));
//        AcquireTable(dynamic_cast<CroTable*>(newTable));
//        return newTable;
//    }
//
//    bool IsBankAcquired(ICentaurusBank* bank) override;
//    void ReleaseTable(CroTable* table) override;
//
//    void Release() override;
//
//    centaurus_size GetMemoryUsage() override;
//private:
//    boost::atomic<float> m_fTaskProgress;
//    CentaurusRun m_RunFunction;
//
//    boost::mutex m_DataLock;
//    std::vector<ICentaurusBank*> m_Banks;
//    std::vector<std::unique_ptr<CroTable>> m_Tables;
//};

class CentaurusTask : public ICentaurusTask
{
public:
    void RunTask() override;
    void Release() override;
    centaurus_size GetMemoryUsage() override;
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

#endif