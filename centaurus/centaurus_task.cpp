#include "centaurus_task.h"
#include <json_file.h>
#include <win32util.h>
#include <croexception.h>
#include <crofile.h>


CentaurusTask::~CentaurusTask()
{
    Release();
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
    if (centaurus->IsBankAcquired(bank)) return false;
    {
        auto lock = boost::mutex::scoped_lock(m_DataLock);
        m_Banks.push_back(bank);
    }

    return bank->Connect();
}

void CentaurusTask::AcquireTable(CroTable* table)
{
    auto lock = boost::mutex::scoped_lock(m_DataLock);
    centaurus->LogTable(*table);
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
