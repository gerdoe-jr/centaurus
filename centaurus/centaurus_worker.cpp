#include "centaurus_worker.h"
#include <stdexcept>

/* CentaurusWorker */

CentaurusWorker::CentaurusWorker()
{
    m_bRunning = false;
}

CentaurusWorker::~CentaurusWorker()
{
}

void CentaurusWorker::StartTask()
{
    Start();
    CentaurusTask::StartTask();
}

void CentaurusWorker::EndTask()
{
    Stop();
    CentaurusTask::EndTask();
}

void CentaurusWorker::Run()
{
    do {
        {
            //boost::mutex::scoped_lock lock(m_Lock);
            boost::unique_lock<boost::mutex> lock(m_Lock);
            if (!IsRunning()) break;
            if (IsWaiting()) m_Notify.wait(lock);
        }
        Execute();
    } while (IsRunning());
}

bool CentaurusWorker::IsRunning() const
{
    return m_bRunning;
}

void CentaurusWorker::Start()
{
    m_bRunning = true;
    Sync();
}

void CentaurusWorker::Stop()
{
    m_bRunning = false;
    Sync();
}

void CentaurusWorker::Sync()
{
    m_Notify.notify_all();
}

bool CentaurusWorker::IsWaiting()
{
    return false;
}

void CentaurusWorker::Execute()
{
}
