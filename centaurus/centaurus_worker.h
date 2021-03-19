#ifndef __CENTAURUS_WORKER_H
#define __CENTAURUS_WORKER_H

#ifdef CENTAURUS_INTERNAL
#include "centaurus_task.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

//class CentaurusWorker : public CentaurusTask
//{
//public:
//    CentaurusWorker();
//    virtual ~CentaurusWorker();
//
//    virtual void StartTask();
//    virtual void EndTask();
//    //virtual void Interrupt();
//    virtual void Run();
//    
//    bool IsRunning() const;
//    virtual void Start();
//    virtual void Stop();
//    void Sync();
//protected:
//    virtual bool IsWaiting();
//    virtual void Execute();
//
//    boost::mutex m_Lock;
//private:
//    boost::condition_variable m_Notify;
//    bool m_bRunning;
//};

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

#endif