#include "centaurus_task.h"
#include <croexception.h>

CCentaurusTask::CCentaurusTask()
    : m_fTaskProgress(0)
{
}

CCentaurusTask::CCentaurusTask(CentaurusRun run)
    : m_fTaskProgress(0), m_RunFunction(run)
{
}

CCentaurusTask::~CCentaurusTask()
{
}

void CCentaurusTask::StartTask()
{
    UpdateProgress(0);

    try {
        Run();
    }
    catch (const boost::thread_interrupted& ti) {
        Interrupt();
    }
    catch (CroException& cro) {
        fprintf(stderr, "CroFile(%p): %s\n", cro.File(), cro.what());
    }
    catch (const std::exception& e) {
        fprintf(stderr, "CCentaurusTask(%p): %s\n", this, e.what());
    }

    EndTask();
}

void CCentaurusTask::EndTask()
{
    UpdateProgress(100);
    centaurus->EndTask(this);
}

void CCentaurusTask::Interrupt()
{
    EndTask();
}

void CCentaurusTask::Run()
{
    if (m_RunFunction)
        m_RunFunction(this);
}

float CCentaurusTask::GetTaskProgress() const
{
    return m_fTaskProgress;
}

void CCentaurusTask::UpdateProgress(float progress)
{
    m_fTaskProgress = progress;
    centaurus->TaskNotify(this);
}

bool CCentaurusTask::AcquireBank(ICentaurusBank* bank)
{
    if (centaurus->IsBankAcquired(bank))
        return false;

    auto lock = std::unique_lock<boost::mutex>(m_DataLock);
    m_Banks.emplace_back(bank);

    return true;
}

void CCentaurusTask::AcquireTable(CroTable* table)
{
    auto lock = std::unique_lock<boost::mutex>(m_DataLock);
    m_Tables.emplace_back(table);
}

CroTable* CCentaurusTask::AcquireTable(CroTable&& table)
{
    auto lock = std::unique_lock<boost::mutex>(m_DataLock);
    return m_Tables.emplace_back(
        std::make_unique<CroTable>(std::move(table))).get();
}

bool CCentaurusTask::IsBankAcquired(ICentaurusBank* bank)
{
    auto lock = std::unique_lock<boost::mutex>(m_DataLock);
    return std::find(m_Banks.begin(), m_Banks.end(), bank)
        != m_Banks.end();
}

void CCentaurusTask::ReleaseTable(CroTable* table)
{
    auto lock = std::unique_lock<boost::mutex>(m_DataLock);
    for (auto it = m_Tables.begin(); it != m_Tables.end(); it++)
    {
        if (it->get() == table)
        {
            m_Tables.erase(it);
            return;
        }
    }
}

centaurus_size CCentaurusTask::GetMemoryUsage()
{
    auto lock = std::unique_lock<boost::mutex>(m_DataLock);

    centaurus_size total = 0;
    for (const auto& table : m_Tables)
        total += table->GetSize();
    return total;
}

