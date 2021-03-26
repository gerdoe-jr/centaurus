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

CentaurusAPI* _centaurus = NULL;

/* CentaurusAPI */

void CentaurusAPI::Init(const std::wstring& path)
{
    m_fOutput = stdout;
    m_fError = stderr;

    PrepareDataPath(path);
    m_TableSizeLimit = 512 * 1024 * 1024; //512 MB
    m_uWorkerLimit = 4; //4 threads

    m_pLoader = std::make_unique<CentaurusLoader>();
    m_pScheduler = std::make_unique<CentaurusScheduler>();

    m_pScheduler->SetPoolSize(m_uWorkerLimit);
    m_ExportIndex = json::object();

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

void CentaurusAPI::SetWorkerLimit(unsigned count)
{
    m_uWorkerLimit = count;
    m_pScheduler->SetPoolSize(m_uWorkerLimit);
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
    {
        auto it = std::find(m_FailedBanks.begin(), m_FailedBanks.end(), path);
        if (it != m_FailedBanks.end()) return NULL;

        m_SyncCond.wait(lock);
    }

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
    auto lock = scoped_lock(m_LogLock);
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
    auto lock = scoped_lock(m_LogLock);

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
        auto lock = scoped_lock(m_LogLock);

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
        OnException(e);
    }
}

void CentaurusAPI::StartTask(ICentaurusTask* task)
{
    {
        auto lock = scoped_lock(m_LogLock);
        fprintf(m_fOutput, "[CentaurusAPI] StartTask %p\n", task);
    }

    TaskSyncJSON(task, {
        {"progress", 0.0f},
        {"memoryUsage", task->GetMemoryUsage()}
    });

    {
        auto lock = scoped_lock(m_TaskLock);
        m_Tasks.emplace_back(task);
    }

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
    
    if (worker == m_pLoader.get())
    {
        auto bankLock = scoped_lock(m_BankLock);
        
        std::wstring dir = m_pLoader->m_LoadedPath;
        ICentaurusBank* bank = m_pLoader->m_pLoadedBank;

        if (bank)
        {
            m_Banks.emplace_back(bank);
            fprintf(m_fOutput, "[CentaurusAPI] Loaded bank \"%s\"\n",
                WcharToAnsi(dir, 866).c_str());
        }
        else
        {
            m_FailedBanks.push_back(dir);
            fprintf(m_fError,
                "[CentaurusAPI] Failed to load bank at \"%s\"\n",
                WcharToAnsi(dir, 866).c_str()
            );
        }
    }

    m_SyncCond.notify_all();
    //fprintf(m_fOutput, "[CentaurusAPI] Sync %s\n",
    //    _worker->GetName().c_str());
}

void CentaurusAPI::OnException(const std::exception& exc)
{
    auto lock = scoped_lock(m_LogLock);
    fprintf(m_fError, "[CentaurusAPI] %s\n", exc.what());
#ifdef CENTAURUS_DEBUG
    //throw exc;
#endif
}

void CentaurusAPI::OnWorkerException(ICentaurusWorker* worker,
    const std::exception& exc)
{
    auto lock = scoped_lock(m_LogLock);
    auto* _worker = dynamic_cast<CentaurusWorker*>(worker);

    fprintf(m_fError, "CentaurusWorker(%s) %s\n",
        _worker->GetName().c_str(), exc.what());
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

bool CentaurusAPI::IsBankExported(uint64_t bankId)
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

void CentaurusAPI::UpdateBankExportIndex(uint64_t bankId,
    const std::wstring& path)
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
