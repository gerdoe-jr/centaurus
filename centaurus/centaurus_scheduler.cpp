#include "centaurus_scheduler.h"

void CentaurusScheduler::ScheduleTask(ICentaurusTask* task)
{
    auto lock = scoped_lock(m_Lock);
    CentaurusJob* job = m_Jobs.emplace_back(
        new CentaurusJob(task)).get();

    job->Start();

    printf("[Scheduler] ScheduleTask %p -> %s\n",
        task, job->GetName().c_str());

    m_Cond.notify_one();
}

std::string CentaurusScheduler::TaskName(ICentaurusTask* task)
{
    auto lock = scoped_lock(m_Lock);

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
        auto lock = scoped_lock(m_Lock);
        for (unsigned i = 0; i < m_Jobs.size();)
        {
            auto it = m_Jobs.begin() + i;
            auto job = it->get();

            if (job->State() == ICentaurusWorker::Terminated)
            {
                printf("[Scheduler] %s terminated.\n",job->GetName().c_str());
                ICentaurusTask* task = job->JobTask();

                centaurus->ReleaseTask(task);
                m_Jobs.erase(it);
            }
            else
            {
                //printf("[Scheduler] %s state %u\n",
                //    job->GetName().c_str(), job->State());
            }
        }
    }
}
