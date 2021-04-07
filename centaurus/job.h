#ifndef __CENTAURUS_JOB_H
#define __CENTAURUS_JOB_H

#include "worker.h"

class CentaurusJob : public CentaurusWorker
{
public:
    CentaurusJob(ICentaurusTask* task);
    virtual ~CentaurusJob();

    void Execute() override;

    ICentaurusTask* JobTask();
protected:
    ICentaurusTask* m_pTask;
};

#endif