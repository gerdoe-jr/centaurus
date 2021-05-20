#include "scheduler.h"

CentaurusScheduler::CentaurusScheduler(unsigned poolSize)
{
    m_uPoolSize = poolSize;
}

void CentaurusScheduler::SetPoolSize(unsigned poolSize)
{
    auto lock = scoped_lock(m_DataLock);
    m_uPoolSize = poolSize;
}

void CentaurusScheduler::ScheduleTask(ICentaurusTask* task)
{
    auto dataLock = scoped_lock(m_DataLock);
    while (m_Jobs.size() >= m_uPoolSize)
        m_DataCond.wait(dataLock);

    {
        //auto lock = scoped_lock(m_DataLock);
        CentaurusJob* job = m_Jobs.emplace_back(
            new CentaurusJob(task)).get();
        job->SetMemoryLimit(centaurus->GetWorkerMemoryLimit());
        
        job->Start();
        Log("ScheduleTask %p -> %s\n", task, job->GetName().c_str());
    }

    m_SyncCond.notify_one();
}

std::string CentaurusScheduler::TaskName(ICentaurusTask* task)
{
    auto lock = scoped_lock(m_DataLock);

    for (auto& job : m_Jobs)
    {
        if (job->JobTask() == task)
            return job->GetName();
    }

    return GetName();
}

void CentaurusScheduler::Execute()
{
    if (m_Jobs.empty())
    {
        m_State = Waiting;
        return;
    }

    {
        auto lock = scoped_lock(m_DataLock);
        for (unsigned i = 0; i < m_Jobs.size();)
        {
            auto it = m_Jobs.begin() + i;
            auto job = it->get();

            if (job->State() == ICentaurusWorker::Terminated)
            {
                Log("%s terminated.\n", job->GetName().c_str());
                ICentaurusTask* task = job->JobTask();

                centaurus->ReleaseTask(task);
                
                m_Jobs.erase(it);
                m_DataCond.notify_all();
            }
            else i++;
        }
    }
}
