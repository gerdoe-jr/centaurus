#include "centaurus_worker.h"
#include <stdexcept>

#include <boost/lexical_cast.hpp>

/* CentaurusWorker */

CentaurusWorker::CentaurusWorker()
{
    m_State = Waiting;
}

CentaurusWorker::~CentaurusWorker()
{
}

void CentaurusWorker::Start()
{
    m_Thread = boost::thread(&CentaurusWorker::Run, this);

    m_State = Running;
    m_Cond.notify_all();
}

void CentaurusWorker::Stop()
{
    m_State = Terminated;
    m_Cond.notify_all();

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

void CentaurusWorker::Run()
{
    while (m_State != Terminated)
    {
        try {
            if (m_State == Waiting)
            {
                boost::unique_lock<boost::mutex> lock(m_Lock);
                m_Cond.wait(lock);

                m_State = Running;
                continue;
            }
            else if (m_State == Running)
            {
                Execute();
            }
        } catch (const boost::thread_interrupted& ti) {
            m_State = Terminated;
        } catch (const std::exception& e) {
            fprintf(stderr, "[CentaurusWorker] %s\n", e.what());
        }
    }
}

std::string CentaurusWorker::GetName()
{
    return "thread-" + boost::lexical_cast<std::string>(m_Thread.get_id());
}
