#ifndef __CENTAURUS_SCHEDULER_H
#define __CENTAURUS_SCHEDULER_H

#include "centaurus_worker.h"
#include "centaurus_job.h"

class CentaurusScheduler : public CentaurusWorker
{
public:
    CentaurusScheduler(unsigned poolSize = 8);
    
    void SetPoolSize(unsigned poolSize);
    void ScheduleTask(ICentaurusTask* task);
    std::string TaskName(ICentaurusTask* task);
protected:
    void Execute() override;
private:
    std::vector<std::unique_ptr<CentaurusJob>> m_Jobs;
    unsigned m_uPoolSize;
};

#endif