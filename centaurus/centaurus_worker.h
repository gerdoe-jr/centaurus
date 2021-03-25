#ifndef __CENTAURUS_WORKER_H
#define __CENTAURUS_WORKER_H

#include "centaurus_task.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
using scoped_lock = boost::mutex::scoped_lock;

class CentaurusWorker : public ICentaurusWorker
{
public:
    CentaurusWorker();
    virtual ~CentaurusWorker();

    virtual void Start();
    virtual void Stop();
    
    void Wait() override;
    state State() const override;
    
    void Run();
    std::string GetName();
protected:
    virtual void Execute() = 0;

    boost::mutex m_Lock;
    boost::condition_variable m_Cond;
    boost::atomic_uint32_t m_State;
private:
    boost::thread m_Thread;
};

#endif