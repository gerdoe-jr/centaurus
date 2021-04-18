#include "api.h"
#include "bank.h"
#include "worker.h"
#include "export.h"

#include "crofile.h"
#include "crobuffer.h"
#include "cronos_abi.h"
#include "croexception.h"
#include "win32util.h"
#include "json_file.h"
#include <functional>
#include <stdexcept>
#include <exception>
#include <memory>
#include <queue>
#include <tuple>
#include <stdio.h>

#ifdef min
#undef min
#endif

#ifndef WIN32
#define CP_UTF7 65000
#define CP_UTF8 65001
#endif

#include <algorithm>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

CentaurusAPI* _centaurus = NULL;

/* CentaurusAPI */

CentaurusAPI::CentaurusAPI()
    : CentaurusLogger("CentaurusAPI", stdout, stderr)
{
}

CentaurusAPI::~CentaurusAPI()
{
}

void CentaurusAPI::Init(const std::wstring& path)
{
    PrepareDataPath(path);
    
    m_pScheduler = CreateWorker<CentaurusScheduler>();
    m_pFetch = CreateWorker<CentaurusFetch>();

    SetTableSizeLimit(CENTAURUS_API_TABLE_LIMIT);
    SetWorkerLimit(CENTAURUS_API_WORKER_LIMIT);
}

void CentaurusAPI::Exit()
{
    m_pFetch->Stop();
    m_pScheduler->Stop();

    m_Tasks.clear();
    m_Banks.clear();
}

void CentaurusAPI::SetTableSizeLimit(centaurus_size limit)
{
    m_TableSizeLimit = limit;
}

void CentaurusAPI::SetWorkerLimit(unsigned count)
{
    m_uWorkerLimit = count;
    m_pScheduler->SetPoolSize(m_uWorkerLimit);
}

void CentaurusAPI::PrepareDataPath(const std::wstring& path)
{
    m_DataPath = path;
    m_ExportIndex = json::object();
    fs::create_directories(m_DataPath);

    fs::create_directory(GetExportPath());
    fs::create_directory(GetTaskPath());
    fs::create_directory(GetBankPath());
}

std::wstring CentaurusAPI::GetExportPath() const
{
    return m_DataPath + L"\\export";
}

std::wstring CentaurusAPI::GetTaskPath() const
{
    return m_DataPath + L"\\task";
}

std::wstring CentaurusAPI::GetBankPath() const
{
    return m_DataPath + L"\\bank";
}

void CentaurusAPI::StartWorker(ICentaurusWorker* worker)
{
    worker->SetWorkerLogger(this);
    worker->Start();
}

void CentaurusAPI::StopWorker(ICentaurusWorker* worker)
{
    if (worker) worker->Stop();
}

std::wstring CentaurusAPI::BankFile(ICentaurusBank* bank)
{
    for (const auto& _bank : m_Banks)
    {
        if (_bank.get() == bank)
        {
            return GetBankPath() + L"\\" + std::to_wstring(
                bank->BankId()) + L".json";
        }
    }

    return L"";
}

void CentaurusAPI::ConnectBank(const std::wstring& path)
{
    m_pFetch->RequestBank(path);
}

void CentaurusAPI::DisconnectBank(ICentaurusBank* bank)
{
    auto lock = boost::unique_lock<boost::mutex>(m_BankLock);
    fs::remove(BankFile(bank));

    for (auto it = m_Banks.begin(); it != m_Banks.end(); it++)
    {
        auto* theBank = it->get();
        if (theBank == bank)
        {
            if (IsBankAcquired(theBank))
                throw std::runtime_error("bank is acquired");
            m_Banks.erase(it);
            return;
        }
    }
}

ICentaurusBank* CentaurusAPI::FindBank(const std::wstring& path)
{
    for (auto& bank : m_Banks)
    {
        auto* _bank = bank->Bank();
        if (!_bank->GetBankPath().compare(path)) return bank.get();
    }

    return NULL;
}

ICentaurusBank* CentaurusAPI::WaitBank(const std::wstring& path)
{
    ICentaurusBank* bank = NULL;
    boost::unique_lock<boost::mutex> lock(m_SyncLock);

    while (!(bank = FindBank(path)))
    {
        auto it = std::find(m_FailedBanks.begin(), m_FailedBanks.end(), path);
        if (it != m_FailedBanks.end()) return NULL;

        m_SyncCond.wait(lock);
    }

    return bank;
}

void CentaurusAPI::ExportABIHeader(const CronosABI* abi, FILE* out)
{
    if (!out) out = LogFile(LogOutput);
    cronos_abi_num abiVersion = abi->GetABIVersion();
    
    fprintf(out, "// Cronos %dx %s %s, ABI %02d.%02d\n\n",
        abi->GetVersion(),
        abi->IsLite() ? "Lite" : "Pro",
        abi->GetModel() == cronos_model_big ? "big model" : "small model",
        abiVersion.first, abiVersion.second
    );

    cronos_rel member_off = 0;
    for (unsigned i = 0; i < cronos_last; i++)
    {
        cronos_value value = (cronos_value)i;
        const auto* pValue = abi->GetValue(value);

        const char* valueType;
        switch (pValue->m_ValueType)
        {
        case cronos_value_data: valueType = "uint8_t"; break;
        case cronos_value_struct: valueType = "CroData"; break;
        case cronos_value_uint16: valueType = "uint16_t"; break;
        case cronos_value_uint32: valueType = "uint32_t"; break;
        case cronos_value_uint64: valueType = "uint64_t"; break;
        }

        switch (pValue->m_FileType)
        {
        case CRONOS_MEM:
            fprintf(out,
                "%s CroData(NULL, cronos_id, (const uint8_t*)%p, %"
                FCroSize ");\n",
                abi->GetValueName(value),
                pValue->m_pMem,
                pValue->m_Size
            );
            break;
        case CRONOS_TAD:
        case CRONOS_DAT:
            if (pValue->m_ValueType == cronos_value_struct)
            {
                if (member_off)
                {
                    fprintf(out, "};\n\n");
                    member_off = 0;
                }

                fprintf(out, "struct %s /* %" FCroSize " */ {\n",
                    abi->GetValueName(value),
                    pValue->m_Size
                );
            }
            else
            {
                const char* valueName = abi->GetValueName(value) + 7;
                if (pValue->m_Offset > member_off)
                {
                    unsigned skip = pValue->m_Offset - member_off;
                    fprintf(out, "\tuint8_t __skip%" FCroOff
                        "[%u];\n\n", member_off, skip);
                    member_off += skip;
                }

                if (pValue->m_Offset < member_off)
                {
                    fprintf(out,
                        "\t/* 0x%" FCroOff " %" FCroSize " & 0x%"
                        PRIX64 " */\n",
                        pValue->m_Offset,
                        pValue->m_Size,
                        pValue->m_Mask
                    );
                }
                else if (pValue->m_ValueType == cronos_value_data)
                {
                    fprintf(out, "\t%s %s[%" FCroSize "];\n",
                        valueType, valueName, pValue->m_Size
                    );
                }
                else
                {
                    fprintf(out, "\t%s %s; /* & 0x%" PRIx64 " */\n",
                        valueType, valueName, pValue->m_Mask
                    );
                }

                member_off += pValue->m_Size;
            }

            break;
        }
    }

    if (member_off) fprintf(out, "};\n");
    fprintf(out, "\n");
}

#include <boost/lexical_cast.hpp>

std::wstring CentaurusAPI::TaskFile(ICentaurusTask* task)
{
    for (auto& _task : m_Tasks)
    {
        if (_task.get() == task)
        {
            return GetTaskPath() + L"\\" + AnsiToWchar(
                m_pScheduler->TaskName(task)) + L".json";
        }
    }

    return L"";
}

void CentaurusAPI::TaskSyncJSON(ICentaurusTask* task, json value)
{
    json taskJson;
    std::wstring jsonPath = TaskFile(task);

    std::wstring dump = TextToWchar(value.dump());
    Log("TaskSyncJSON %s\n", WcharToAnsi(dump, 866).c_str());

    try { taskJson = ReadJSONFile(jsonPath); }
    catch (const std::exception& e) { taskJson = json::object(); }

    taskJson.insert(value.begin(), value.end());

    try {
        WriteJSONFile(jsonPath, taskJson);
    } catch (const std::exception& e) { 
        OnException(e);
    }
}

void CentaurusAPI::StartTask(ICentaurusTask* task)
{
    Log("StartTask %p\n", task);
    {
        auto lock = scoped_lock(m_TaskLock);
        m_Tasks.emplace_back(task);
    }

    TaskSyncJSON(task, {
        {"progress", 0.0f},
        {"memoryUsage", task->GetMemoryUsage()}
    });

    m_pScheduler->ScheduleTask(task);
}

void CentaurusAPI::ReleaseTask(ICentaurusTask* task)
{
    auto lock = scoped_lock(m_TaskLock);
    for (auto it = m_Tasks.begin(); it != m_Tasks.end(); it++)
    {
        if (it->get() == task)
        {
            m_Tasks.erase(it);
            break;
        }
    }
}

bool CentaurusAPI::IsBankLoaded(ICentaurusBank* bank)
{
    auto lock = boost::unique_lock<boost::mutex>(m_BankLock);
    for (const auto& _bank : m_Banks)
        if (_bank.get() == bank) return true;
    return false;
}

bool CentaurusAPI::IsBankAcquired(ICentaurusBank* bank)
{
    auto lock = boost::unique_lock<boost::mutex>(m_TaskLock);
    for (auto& task : m_Tasks)
    {
        if (task->IsBankAcquired(bank))
            return true;
    }

    return false;
}

centaurus_size CentaurusAPI::TotalMemoryUsage()
{
    auto lock = scoped_lock(m_TaskLock);

    centaurus_size total = 0;
    for (auto& task : m_Tasks)
        total += task->GetMemoryUsage();
    return total;
}

centaurus_size CentaurusAPI::RequestTableLimit()
{
    centaurus_size ramUsage = TotalMemoryUsage();
    if (ramUsage > m_TableSizeLimit) return m_TableSizeLimit;

    cronos_size available = m_TableSizeLimit - ramUsage;
    return std::min(m_TableSizeLimit, available / 2);
}

void CentaurusAPI::Run()
{
    /*for (auto& bank : m_Banks)
    {
        fprintf(m_fOutput, "[CentaurusAPI::Run] Detect \"%s\", ID "
            "%" PRIu64 "\n", WcharToAnsi(bank->BankName(), 866).c_str(),
                bank->BankId());

        CentaurusExport* taskExport = new CentaurusExport;
        taskExport->SetTargetBank(bank.get());

        //StartTask(taskExport);
        m_Tasks.emplace_back(taskExport);

        m_pScheduler->ScheduleTask(taskExport);
    }*/

    m_pScheduler->Wait();
}

void CentaurusAPI::Sync(ICentaurusWorker* worker)
{
    auto lock = scoped_lock(m_SyncLock);
    auto* _worker = dynamic_cast<CentaurusWorker*>(worker);
    
    if (worker == m_pFetch.get())
    {
        auto bankLock = scoped_lock(m_BankLock);
        
        std::wstring dir = m_pFetch->m_LoadedPath;
        ICentaurusBank* bank = m_pFetch->m_pLoadedBank;

        m_pFetch->m_LoadedPath = L"";
        m_pFetch->m_pLoadedBank = NULL;

        if (bank)
        {
            m_Banks.emplace_back(bank);
            Log("Loaded bank \"%s\"\n", WcharToAnsi(dir, 866).c_str());
        }
        else if (!dir.empty())
        {
            m_FailedBanks.push_back(dir);
            Error("Failed to load bank at \"%s\"\n",
                WcharToAnsi(dir, 866).c_str());
        }
    }

    m_SyncCond.notify_all();
    //fprintf(m_fOutput, "[CentaurusAPI] Sync %s\n",
    //    _worker->GetName().c_str());
}

void CentaurusAPI::OnException(const std::exception& exc)
{
    Error(exc.what());
#ifdef CENTAURUS_DEBUG
    //throw exc;
#endif
}

void CentaurusAPI::OnWorkerException(ICentaurusWorker* worker,
    const std::exception& exc)
{
    auto* _worker = dynamic_cast<CentaurusWorker*>(worker);
#ifdef CENTAURUS_DEBUG
    //throw exc;
#endif
}

std::string CentaurusAPI::SizeToString(centaurus_size size) const
{
    const std::string suffix[] = { "B", "KB", "MB", "GB" };
    unsigned i = 0;

    double bytes = (double)size;
    while (bytes >= 1024.0)
    {
        bytes /= 1024.0;
        i++;
    }

    return std::to_string((unsigned)bytes) + " " + suffix[i];
}

bool CentaurusAPI::IsBankExported(uint32_t bankId)
{
    auto lock = scoped_lock(m_ExportLock);
    std::wstring indexPath = GetExportPath() + L"\\index.json";
    try {
        if (fs::exists(indexPath)) m_ExportIndex = ReadJSONFile(indexPath);
        else m_ExportIndex = json::object();
    }
    catch (const std::exception& e) {
        OnException(e);
    }

    return m_ExportIndex.find(std::to_string(bankId)) != m_ExportIndex.end();
}

void CentaurusAPI::UpdateBankExportIndex(uint32_t bankId, const std::wstring& path)
{
    auto lock = scoped_lock(m_ExportLock);
    std::wstring indexPath = GetExportPath() + L"\\index.json";
    try {
        if (fs::exists(indexPath))
        {
            if (m_ExportIndex.empty())
                m_ExportIndex = ReadJSONFile(indexPath);
        }
        
        m_ExportIndex[std::to_string(bankId)] = WcharToText(path);
        WriteJSONFile(indexPath, m_ExportIndex);
    }
    catch (const std::exception& e) {
        OnException(e);
    }
}
