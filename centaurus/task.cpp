#include "task.h"
#include <json_file.h>
#include <win32util.h>
#include <croexception.h>
#include <crofile.h>

CentaurusTask::CentaurusTask()
    : CentaurusLogger("CentaurusTask", logger),
    m_TaskName("CentaurusTask")
{
    m_pInvoker = NULL;
    m_Limit = 0;
}

CentaurusTask::CentaurusTask(const std::string& taskName)
    : CentaurusLogger(taskName, logger),
    m_TaskName(taskName)
{
    m_pInvoker = NULL;
    m_Limit = 0;
}

CentaurusTask::~CentaurusTask()
{
    Release();
}

void CentaurusTask::Invoke(ICentaurusWorker* invoker)
{
    m_pInvoker = invoker;
    SetLogForward(m_pInvoker ? m_pInvoker->GetWorkerLogger() : logger);
    m_Limit = invoker->GetMemoryLimit();
}

void CentaurusTask::RunTask()
{
}

void CentaurusTask::Release()
{
    auto lock = boost::mutex::scoped_lock(m_DataLock);
    for (auto& bank : m_Banks) bank->Disconnect();
    
    m_Banks.clear();
    m_Tables.clear();
}

centaurus_size CentaurusTask::GetMemoryUsage()
{
    centaurus_size total = 0;
    auto lock = boost::mutex::scoped_lock(m_DataLock);
    for (const auto& table : m_Tables)
        total += table->GetSize();
    return total;
}

bool CentaurusTask::AcquireBank(ICentaurusBank* bank)
{
    if (centaurus->IsBankAcquired(bank))
        return false;
    
    bool connected = bank->Connect();
    if (connected)
    {
        auto lock = boost::mutex::scoped_lock(m_DataLock);
        m_Banks.push_back(bank);
    }

    return connected;
}

void CentaurusTask::AcquireTable(CroTable* table)
{
    auto lock = boost::mutex::scoped_lock(m_DataLock);
    LogTable(*table);
    m_Tables.emplace_back(table);
}

CroTable* CentaurusTask::AcquireTable(CroTable&& table)
{
    CroTable* newTable = new CroTable(std::move(table));
    AcquireTable(newTable);
    return newTable;
}

bool CentaurusTask::IsBankAcquired(ICentaurusBank* bank)
{
    auto lock = boost::mutex::scoped_lock(m_DataLock);
    auto it = std::find(m_Banks.begin(), m_Banks.end(), bank);

    return it != m_Banks.end();
}

void CentaurusTask::ReleaseTable(CroTable* table)
{
    for (auto it = m_Tables.begin(); it != m_Tables.end(); it++)
    {
        if (it->get() == table)
        {
            m_Tables.erase(it);
            break;
        }
    }
}

ICentaurusWorker* CentaurusTask::Invoker() const
{
    return m_pInvoker;
}

const std::string& CentaurusTask::TaskName() const
{
    return m_TaskName;
}
