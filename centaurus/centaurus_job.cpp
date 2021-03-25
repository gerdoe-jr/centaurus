#include "centaurus_job.h"
#include "centaurus_api.h"
#include <croexception.h>
#include <crofile.h>
#include <win32util.h>
#include <stdexcept>

CentaurusJob::CentaurusJob(ICentaurusTask* task)
    : m_pTask(task)
{
}

CentaurusJob::~CentaurusJob()
{
}

void CentaurusJob::Execute()
{
    try {
        m_pTask->RunTask();
    }
    catch (std::exception& e) {
        centaurus->OnException(e);
        _centaurus->TaskSyncJSON(m_pTask, {
            {"error", {
                {"exception", "std::exception"},
                {"what", e.what()}
            }}
        });
    }

    m_State = Terminated;
}

ICentaurusTask* CentaurusJob::JobTask()
{
    return m_pTask;
}
