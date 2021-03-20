#include "centaurus_api.h"
#include "centaurus_bank.h"
#include "centaurus_worker.h"
#include "centaurus_export.h"

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

/* CentaurusJob */

CentaurusJob::CentaurusJob(ICentaurusTask* task)
    : m_pTask(task)
{
}

CentaurusJob::~CentaurusJob()
{
    m_pTask->Release();
}

void CentaurusJob::Execute()
{
    try {
        m_pTask->RunTask();
    } catch (CroException& ce) {
        ((CentaurusAPI*)centaurus)->TaskSyncJSON(m_pTask, {
            {"error", {
                {"exception", "CroException"},
                {"file", WcharToText(ce.File()->GetPath())},
                {"what", ce.what()}
            }}
        });
    } catch (const std::exception& ce) {
        ((CentaurusAPI*)centaurus)->TaskSyncJSON(m_pTask, {
            {"error", {
                {"exception", "std::exception"},
                {"what", ce.what()}
            }}
        });
    }

    m_State = Terminated;
}

ICentaurusTask* CentaurusJob::JobTask()
{
    return m_pTask;
}

/* CentaurusLoader */

void CentaurusLoader::RequestBank(const std::wstring& path)
{
    {
        auto lock = boost::mutex::scoped_lock(m_Lock);
        m_Dirs.push(path);
    }
    m_Cond.notify_one();
}

void CentaurusLoader::RequestBanks(std::vector<std::wstring> dirs)
{
    {
        auto lock = boost::mutex::scoped_lock(m_Lock);
        for (const auto& dir : dirs) m_Dirs.push(dir);
    }
    m_Cond.notify_one();
}

void CentaurusLoader::Execute()
{
    if (m_Dirs.empty())
    {
        m_State = Waiting;
        return;
    }

    std::wstring dir;
    {
        auto lock = boost::mutex::scoped_lock(m_Lock);
        dir = m_Dirs.front();
        m_Dirs.pop();
    }

    if (LoadPath(dir))
    {
        m_LoadedPath = dir;
        centaurus->Sync(this);
    }
    else LogLoaderFail(dir);
}

bool CentaurusLoader::LoadPath(const std::wstring& path)
{
    auto exp = std::make_unique<CentaurusExport>();
    CentaurusBank* bank = new CentaurusBank();
    m_pLoadedBank = NULL;

    bank->AssociatePath(path);
    if (!exp->AcquireBank(bank))
    {
        delete bank;
        return false;
    }

    exp->SetTargetBank(bank);
    m_pLoadedBank = bank;
    
    bank->LoadStructure(exp.get());
    bank->LoadBases(exp.get());

    return true;
}

void CentaurusLoader::LogLoaderFail(const std::wstring& dir)
{
    fprintf(stderr, "[CentaurusLoader] failed to load %s\n",
        WcharToAnsi(dir).c_str());
}

/* CentaurusScheduler */

void CentaurusScheduler::ScheduleTask(ICentaurusTask* task)
{
    auto lock = boost::mutex::scoped_lock(m_Lock);
    CentaurusJob* job = m_Jobs.emplace_back(
        new CentaurusJob(task)).get();

    job->Start();

    printf("[Scheduler] ScheduleTask %p -> %s\n",
        task, job->GetName().c_str());

    m_Cond.notify_one();
}

std::string CentaurusScheduler::TaskName(ICentaurusTask* task)
{
    auto lock = boost::mutex::scoped_lock(m_Lock);

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
        auto lock = boost::mutex::scoped_lock(m_Lock);
        for (unsigned i = 0; i < m_Jobs.size();)
        {
            auto it = m_Jobs.begin() + i;
            auto job = it->get();

            if (job->State() == ICentaurusWorker::Terminated)
            {
                printf("[Scheduler] %s terminated.\n",
                    job->GetName().c_str());
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

/* CentaurusAPI */

void CentaurusAPI::Init(const std::wstring& path)
{
    m_fOutput = stdout;
    m_fError = stderr;

    PrepareDataPath(path);
    SetTableSizeLimit(512 * 1024 * 1024); //512 MB

    m_pLoader = std::make_unique<CentaurusLoader>();
    m_pScheduler = std::make_unique<CentaurusScheduler>();

    m_pLoader->Start();
    m_pScheduler->Start();
}

void CentaurusAPI::Exit()
{
    m_pLoader->Stop();
    m_pScheduler->Stop();

    if (m_fOutput != stdout) fclose(m_fOutput);
    if (m_fError != stderr) fclose(m_fError);

    
    m_Tasks.clear();
    m_Banks.clear();
}

void CentaurusAPI::SetTableSizeLimit(centaurus_size limit)
{
    m_TableSizeLimit = limit;
}

void CentaurusAPI::PrepareDataPath(const std::wstring& path)
{
    m_DataPath = path;
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
    m_pLoader->RequestBank(path);
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
        if (!bank->GetPath().compare(path)) return bank.get();
    return NULL;
}

ICentaurusBank* CentaurusAPI::WaitBank(const std::wstring& path)
{
    ICentaurusBank* bank = NULL;
    boost::unique_lock<boost::mutex> lock(m_SyncLock);

    while (!(bank = FindBank(path)))
        m_SyncCond.wait(lock);

    return bank;
}

void CentaurusAPI::ExportABIHeader(const CronosABI* abi, FILE* out) const
{
    if (!out) out = m_fOutput;

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

void CentaurusAPI::LogBankFiles(ICentaurusBank* bank)
{
    auto lock = boost::mutex::scoped_lock(m_LogLock);
    static const char* _croName[] = { "CroStru", "CroBank", "CroIndex" };

    bank->Connect();

    for (unsigned i = 0; i < CroBankFile_Count; i++)
    {
        CroFile* file = bank->File((CroBankFile)i);
        if (!file) continue;

        
        const CronosABI* abi = file->ABI();
        cronos_abi_num abiVersion = abi->GetABIVersion();

        fprintf(m_fOutput,
            "\t[%s] Cronos %dx %s %s, ABI %02d.%02d\n", _croName[i],
            abi->GetVersion(), abi->IsLite() ? "Lite" : "Pro",
            abi->GetModel() == cronos_model_big ? "big model" : "small model",
            abiVersion.first, abiVersion.second
        );
    }
    
    bank->Disconnect();
}

void CentaurusAPI::LogBuffer(const CroBuffer& buf, unsigned codepage)
{
    auto lock = boost::mutex::scoped_lock(m_LogLock);

    const char ascii_lup = 201, ascii_rup = 187,
        ascii_lsp = 199, ascii_rsp = 182,
        ascii_lbt = 200, ascii_rbt = 188,
        ascii_up_cross = 209, ascii_bt_cross = 207,
        ascii_cross = 197,
        ascii_v_sp = 179, ascii_h_sp = 196,
        ascii_v_border = 186, ascii_h_border = 205;

    const cronos_size line = 0x10;

    //code page
#ifdef WIN32
    if (!codepage) codepage = GetConsoleOutputCP();
#endif

    //start
    putc(ascii_lup, m_fOutput);
    for (cronos_rel i = 0; i < 8; i++) putc(ascii_h_border, m_fOutput);
    putc(ascii_up_cross, m_fOutput);
    for (cronos_rel i = 0; i < line * 3 - 1; i++)
        putc(ascii_h_border, m_fOutput);
    putc(ascii_up_cross, m_fOutput);
    for (cronos_rel i = 0; i < line; i++)
        putc(ascii_h_border, m_fOutput);
    putc(ascii_rup, m_fOutput);

    putc('\n', m_fOutput);

    //header
    putc(ascii_v_border, m_fOutput);
    fprintf(m_fOutput, " offset ");
    putc(ascii_v_sp, m_fOutput);
    for (cronos_rel i = 0; i < line; i++)
        fprintf(m_fOutput, i < line - 1 ? "%02x " : "%02x", i & 0xFF);
    putc(ascii_v_sp, m_fOutput);
    switch (codepage)
    {
    case CP_UTF7: fprintf(m_fOutput, " UTF-7  "); break;
    case CP_UTF8: fprintf(m_fOutput, " UTF-8  "); break;
    default: fprintf(m_fOutput, " ANSI CP #%05d ", codepage);
    }
    putc(ascii_v_border, m_fOutput);

    putc('\n', m_fOutput);

    //split
    putc(ascii_lsp, m_fOutput);
    for (cronos_rel i = 0; i < 8; i++)
        putc(ascii_h_sp, m_fOutput);
    putc(ascii_cross, stdout);
    for (cronos_rel i = 0; i < line * 3 - 1; i++)
        putc(ascii_h_sp, m_fOutput);
    putc(ascii_cross, m_fOutput);
    for (cronos_rel i = 0; i < line; i++)
        putc(ascii_h_sp, m_fOutput);
    putc(ascii_rsp, m_fOutput);

    putc('\n', m_fOutput);

    //hex dump
    for (cronos_size off = 0; off < buf.GetSize(); off += line)
    {
        cronos_size len = std::min(buf.GetSize() - off, line);

        putc(ascii_v_border, m_fOutput);
        fprintf(m_fOutput, "%08" FCroOff, off);
        if (len) putc(ascii_v_sp, m_fOutput);
        else break;

        for (cronos_rel i = 0; i < line; i++)
        {
            if (i < len)
            {
                fprintf(m_fOutput, i < line - 1 ? "%02X " : "%02X",
                    buf.GetData()[off + i] & 0xFF);
            }
            else fprintf(m_fOutput, i < line - 1 ? "   " : "  ");
        }

        putc(ascii_v_sp, m_fOutput);

#ifdef WIN32
        SetConsoleOutputCP(codepage);
#endif
        for (cronos_rel i = 0; i < line; i++)
        {
            if (i < len)
            {
                uint8_t byte = buf.GetData()[off + i];
                putc(byte >= 0x20 ? (char)byte : '.', m_fOutput);
            }
            else putc(' ', m_fOutput);
        }
#ifdef WIN32
        SetConsoleOutputCP(866);
#endif
        putc(ascii_v_border, m_fOutput);

        putc('\n', m_fOutput);
    }

    //end
    putc(ascii_lbt, m_fOutput);

    for (cronos_rel i = 0; i < 8; i++)
        putc(ascii_h_border, m_fOutput);
    putc(ascii_bt_cross, m_fOutput);

    for (cronos_rel i = 0; i < line * 3 - 1; i++)
        putc(ascii_h_border, m_fOutput);

    putc(ascii_bt_cross, m_fOutput);
    for (cronos_rel i = 0; i < line; i++)
        putc(ascii_h_border, m_fOutput);
    putc(ascii_rbt, m_fOutput);

    putc('\n', m_fOutput);
}

void CentaurusAPI::LogTable(const CroTable& table)
{
    /*fprintf(m_fOutput,
        "== %s TABLE 0x%" FCroOff " %" FCroSize " ID "
        "%" FCroId "-%" FCroId " COUNT %" FCroIdx "\n",
        table.GetFileType() == CRONOS_TAD ? "TAD" : "DAT",
        table.TableOffset(), table.TableSize(),
        table.IdStart(), table.IdEnd(),
        table.GetEntryCount()
    );*/
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

    {
        auto lock = boost::mutex::scoped_lock(m_LogLock);

        std::wstring dump = TextToWchar(value.dump());
        fprintf(m_fOutput, "[CentaurusAPI] TaskSyncJSON %s\n",
            WcharToAnsi(dump, 866).c_str());
    }

    try { taskJson = ReadJSONFile(jsonPath); }
    catch (const std::exception& e) { taskJson = json::object(); }

    taskJson.insert(value.begin(), value.end());

    try {
        WriteJSONFile(jsonPath, taskJson);
    } catch (const std::exception& e) { 
        fprintf(m_fError, "[CentaurusAPI] Failed to write task json %s\n",
            WcharToAnsi(jsonPath, 866).c_str());
    }
}

void CentaurusAPI::StartTask(ICentaurusTask* task)
{
    {
        auto lock = boost::mutex::scoped_lock(m_LogLock);
        fprintf(m_fOutput, "[CentaurusAPI] StartTask %p\n", task);
    }

    TaskSyncJSON(task, {
        {"progress", 0.0f},
        {"memoryUsage", task->GetMemoryUsage()}
    });

    auto lock = boost::mutex::scoped_lock(m_TaskLock);
    m_Tasks.emplace_back(task);

    m_pScheduler->ScheduleTask(task);
}

void CentaurusAPI::ReleaseTask(ICentaurusTask* task)
{
    auto lock = boost::mutex::scoped_lock(m_TaskLock);
    for (auto it = m_Tasks.begin(); it != m_Tasks.end(); it++)
    {
        if (it->get() == task)
        {
            m_Tasks.erase(it);
            break;
        }
    }
}

void CentaurusAPI::Run()
{
    //m_pScheduler->Wait();
    while (m_pScheduler->State() != ICentaurusWorker::Terminated)
    {
        auto syncLock = boost::mutex::scoped_lock(m_SyncLock);
        if (m_Banks.empty())
        {
            m_SyncCond.wait(syncLock);
            continue;
        }

        for (auto& bank : m_Banks)
        {
            if (std::find(m_KnownBanks.begin(), m_KnownBanks.end(),
                bank->BankId()) != m_KnownBanks.end())
            {
                continue;
            }

            fprintf(m_fOutput, "[CentaurusAPI::Run] Detect \"%s\", ID "
                "%" PRIu64 "\n", WcharToAnsi(bank->BankName(), 866).c_str(),
                    bank->BankId());
            m_KnownBanks.push_back(bank->BankId());

            
            CentaurusExport* taskExport = new CentaurusExport;
            taskExport->SetTargetBank(bank.get());
            
            StartTask(taskExport);
        }
    }
}

void CentaurusAPI::Sync(ICentaurusWorker* worker)
{
    auto lock = boost::mutex::scoped_lock(m_SyncLock);
    auto* _worker = dynamic_cast<CentaurusWorker*>(worker);
    
    if (worker == m_pLoader.get())
    {
        auto bankLock = boost::mutex::scoped_lock(m_BankLock);
        
        std::wstring dir = m_pLoader->m_LoadedPath;
        ICentaurusBank* bank = m_pLoader->m_pLoadedBank;

        m_Banks.emplace_back(bank);
        fprintf(m_fOutput, "[CentaurusAPI] Loaded bank \"%s\"\n",
            WcharToAnsi(dir, 866).c_str());
    }

    m_SyncCond.notify_all();
    //fprintf(m_fOutput, "[CentaurusAPI] Sync %s\n",
    //    _worker->GetName().c_str());
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
    auto lock = boost::unique_lock<boost::mutex>(m_TaskLock);

    centaurus_size total = 0;
    for (auto& task : m_Tasks)
        total += task->GetMemoryUsage();
    return total;
}

centaurus_size CentaurusAPI::RequestTableLimit()
{
    centaurus_size ramUsage = TotalMemoryUsage();
    if (ramUsage > m_TableSizeLimit)
        throw std::runtime_error("ramUsage > m_TableSizeLimit");
    cronos_size available = m_TableSizeLimit - ramUsage;
    return std::min(available / 4, available);
}
