#include "centaurus_task.h"
#include <json_file.h>
#include <win32util.h>
#include <croexception.h>
#include <crofile.h>

CentaurusTask::CentaurusTask()
    : m_fTaskProgress(0)
{
}

CentaurusTask::CentaurusTask(CentaurusRun run)
    : m_fTaskProgress(0), m_RunFunction(run)
{
}

CentaurusTask::~CentaurusTask()
{
}

void CentaurusTask::StartTask()
{
    UpdateProgress(0);

    try {
        Run();
    } catch (const boost::thread_interrupted& ti) {
        Interrupt();
    } catch (CroException& cro) {
        auto _task = json::object();
        try { _task = ReadJSONFile(centaurus->TaskFile(this)); }
        catch (const std::exception& e) {}

        _task["error"] = {
            {"exception", "CroException"},
            {"what", cro.what()},
            {"file", WcharToText(cro.File()->GetPath())}
        };
        
        try { WriteJSONFile(centaurus->TaskFile(this), _task); }
        catch (const std::exception& e) {}
    } catch (const std::exception& e) {
        auto _task = json::object();
        try { _task = ReadJSONFile(centaurus->TaskFile(this)); }
        catch (const std::exception& e) {}

        _task["error"] = {
            {"exception", "std::exception"},
            {"what", e.what()},
        };
        
        try { WriteJSONFile(centaurus->TaskFile(this), _task); }
        catch (const std::exception& e) {}
    }

    EndTask();
}

void CentaurusTask::EndTask()
{
    UpdateProgress(100);
    Release();

    centaurus->EndTask(this);
}

void CentaurusTask::Interrupt()
{
    Release();
}

void CentaurusTask::Run()
{
    if (m_RunFunction)
        m_RunFunction(this);
}

float CentaurusTask::GetTaskProgress() const
{
    return m_fTaskProgress;
}

void CentaurusTask::UpdateProgress(float progress)
{
    m_fTaskProgress = progress;
    centaurus->TaskNotify(this);
}

bool CentaurusTask::AcquireBank(ICentaurusBank* bank)
{
    if (centaurus->IsBankAcquired(bank))
        return false;

    auto lock = std::unique_lock<boost::mutex>(m_DataLock);
    {
        if (!bank->Connect()) return false;
        m_Banks.emplace_back(bank);
    }

    return true;
}

void CentaurusTask::AcquireTable(CroTable* table)
{
    auto lock = std::unique_lock<boost::mutex>(m_DataLock);
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
    auto lock = std::unique_lock<boost::mutex>(m_DataLock);
    return std::find(m_Banks.begin(), m_Banks.end(), bank)
        != m_Banks.end();
}

void CentaurusTask::ReleaseTable(CroTable* table)
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

void CentaurusTask::Release()
{
    auto lock = std::unique_lock<boost::mutex>(m_DataLock);
    
    for (auto& bank : m_Banks)
        bank->Disconnect();

    m_Banks.clear();
    m_Tables.clear();
}

centaurus_size CentaurusTask::GetMemoryUsage()
{
    auto lock = std::unique_lock<boost::mutex>(m_DataLock);

    centaurus_size total = 0;
    for (const auto& table : m_Tables)
        total += table->GetSize();
    return total;
}

