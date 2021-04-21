#ifndef __CENTAURUS_WORKER_H
#define __CENTAURUS_WORKER_H

#include "task.h"
#include "logger.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
using scoped_lock = boost::mutex::scoped_lock;

class CentaurusWorker : public ICentaurusWorker, protected CentaurusLogger
{
public:
    CentaurusWorker();
    virtual ~CentaurusWorker();

    virtual void Start();
    virtual void Stop();
    
    void Wait() override;
    state State() const override;
    
    void SetWorkerLogger(ICentaurusLogger* log) override;
    ICentaurusLogger* GetWorkerLogger() override;
    
    void SetMemoryLimit(centaurus_size limit) override;
    centaurus_size GetMemoryLimit() const override;

    void Run();
    std::string GetName();
private:
    centaurus_size m_MemoryLimit;
protected:
    virtual void Execute() = 0;

    boost::mutex m_DataLock;
    boost::condition_variable m_DataCond;

    boost::mutex m_SyncLock;
    boost::condition_variable m_SyncCond;

    boost::atomic_uint32_t m_State;
    boost::thread m_Thread;
};

#endif