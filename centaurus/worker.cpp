#include "worker.h"
#include <stdexcept>

#include <boost/lexical_cast.hpp>

/* CentaurusWorker */

CentaurusWorker::CentaurusWorker()
    : CentaurusLogger("CentaurusWorker")
{
    m_State = Waiting;
    m_MemoryLimit = 0;
}

CentaurusWorker::~CentaurusWorker()
{
}

void CentaurusWorker::Start()
{
    m_Thread = boost::thread(&CentaurusWorker::Run, this);

    m_State = Running;
    m_SyncCond.notify_all();
}

void CentaurusWorker::Stop()
{
    m_State = Terminated;
    m_SyncCond.notify_all();

    m_Thread.interrupt();
}

void CentaurusWorker::Wait()
{
    m_Thread.join();
}

ICentaurusWorker::state CentaurusWorker::State() const
{
    return (ICentaurusWorker::state)m_State.load();
}

void CentaurusWorker::SetWorkerLogger(ICentaurusLogger* log)
{
    SetLogForward(log);
}

ICentaurusLogger* CentaurusWorker::GetWorkerLogger()
{
    return dynamic_cast<ICentaurusLogger*>(this);
}

void CentaurusWorker::SetMemoryLimit(centaurus_size limit)
{
    m_MemoryLimit = limit;
}

centaurus_size CentaurusWorker::GetMemoryLimit() const
{
    return m_MemoryLimit;
}

void CentaurusWorker::Run()
{
    SetLogName(GetName());
    
    while (m_State != Terminated)
    {
        try {
            if (m_State == Waiting)
            {
                auto lock = scoped_lock(m_SyncLock);
                m_SyncCond.wait(lock);

                m_State = Running;
                continue;
            }
            else if (m_State == Running)
            {
                Execute();
                
                m_SyncCond.notify_all();
                centaurus->Sync(this);
            }
        } catch (const boost::thread_interrupted& ti) {
            m_State = Terminated;
        } catch (std::exception& e) {
            Error(e.what());
            centaurus->OnWorkerException(this, e);
            m_State = Terminated;
        }
    }
}

std::string CentaurusWorker::GetName()
{
    return "thread-" + boost::lexical_cast<std::string>(m_Thread.get_id());
}
