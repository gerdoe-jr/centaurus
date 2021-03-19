#include "centaurus_api.h"
#include "centaurus_bank.h"
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

/* Centaurus Bank Loader*/

class CentaurusBankLoader : public CentaurusExport
{
public:
    CentaurusBankLoader(ICentaurusBank* bank, const std::wstring& path)
        : CentaurusExport(bank),
        m_BankPath(path)
    {
    }

    void Run() override
    {
        CentaurusBank* bank = (CentaurusBank*)TargetBank();
        bank->AssociatePath(m_BankPath);
        if (!AcquireBank(bank))
            throw std::runtime_error("failed to connect bank");

        bank->LoadStructure(this);
        bank->LoadBases(this);

        Sync();
    }

    void Sync()
    {
        ICentaurusBank* bank = TargetBank();
        WriteJSONFile(centaurus->BankFile(bank), {
            {"name", WcharToText(bank->BankName())},
            {"files", {
                {"CroStru", WcharToText(bank->File(CroStru)->GetPath())},
                {"CroBank", WcharToText(bank->File(CroBank)->GetPath())},
                {"CroIndex", WcharToText(bank->File(CroIndex)->GetPath())}
            }}
        });
    }
private:
    std::wstring m_BankPath;
};

/* CentaurusAPI */

void CentaurusAPI::Init()
{
    m_fOutput = stdout;
    m_fError = stderr;

    SetTableSizeLimit(512 * 1024 * 1024); //512 MB
}

void CentaurusAPI::Exit()
{
    if (m_fOutput != stdout) fclose(m_fOutput);
    if (m_fError != stderr) fclose(m_fError);

    for (auto& task : m_Tasks)
    {
        auto& thread = std::get<1>(task);

        thread.interrupt();
        thread.join();
    }
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

ICentaurusBank* CentaurusAPI::ConnectBank(const std::wstring& path)
{
    ICentaurusBank* bank;
    {
        auto lock = boost::unique_lock<boost::mutex>(m_BankLock);
        bank = m_Banks.emplace_back(new CentaurusBank).get();
    }

    StartTask(new CentaurusBankLoader(bank, path));
    return bank;
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

void CentaurusAPI::WaitBank()
{
    Idle();
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

void CentaurusAPI::LogBankFiles(ICentaurusBank* bank) const
{
    bank->Connect();

    for (unsigned i = 0; i < CroBankFile_Count; i++)
    {
        CroFile* file = bank->File((CroBankFile)i);
        if (!file) continue;

        fprintf(m_fOutput, "CroFile(%s):\n",
            WcharToAnsi(file->GetPath()).c_str()
        );

        const CronosABI* abi = file->ABI();
        cronos_abi_num abiVersion = abi->GetABIVersion();

        fprintf(m_fOutput,
            "\tCronos %dx %s %s, ABI %02d.%02d\n",
            abi->GetVersion(),
            abi->IsLite() ? "Lite" : "Pro",
            abi->GetModel() == cronos_model_big ? "big model" : "small model",
            abiVersion.first, abiVersion.second
        );

        fprintf(m_fOutput,
            "\tTAD FileSize: %" FCroSize "\n"
            "\tDAT FileSize: %" FCroSize "\n",
            file->FileSize(CRONOS_TAD),
            file->FileSize(CRONOS_DAT)
        );

        fprintf(m_fOutput,
            "\tEntryCountFileSize: %" FCroIdx "\n",
            file->EntryCountFileSize()
        );
    }
    
    bank->Disconnect();
}

void CentaurusAPI::LogBuffer(const CroBuffer& buf, unsigned codepage)
{
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
        auto pTask = std::get<0>(_task).get();
        if (pTask == task)
        {
            return GetTaskPath() + L"\\thread-" + boost::lexical_cast
                <std::wstring>(std::get<1>(_task).get_id())
                + L".json";
        }
    }

    return L"";
}

void CentaurusAPI::StartTask(ICentaurusTask* task)
{
    auto lock = boost::unique_lock<boost::mutex>(m_TaskLock);
    m_Tasks.emplace_back(task, boost::thread(
        &ICentaurusTask::StartTask, task));

    WriteJSONFile(TaskFile(task), {
        {"progress", task->GetTaskProgress()},
        {"memoryUsage", task->GetMemoryUsage()}
    });
}

void CentaurusAPI::EndTask(ICentaurusTask* task)
{
    auto lock = boost::unique_lock<boost::mutex>(m_TaskLock);
    for (auto it = m_Tasks.begin(); it != m_Tasks.end(); it++)
    {
        auto pTask = std::get<0>(*it).get();
        if (pTask == task)
        {
            auto jsonFile = TaskFile(task);
            auto data = ReadJSONFile(jsonFile);
            if (data.find("error") == data.end())
                fs::remove(jsonFile);
            else fwprintf(stderr, L"failed task: %s\n", jsonFile.c_str());
            
            m_Tasks.erase(it);
            if (m_Tasks.empty())
                m_TaskCond.notify_all();
            return;
        }
    }

    throw std::runtime_error("centaurus->EndTask with no task");
}

CentaurusAPI::Task* CentaurusAPI::GetTask(ICentaurusTask* task)
{
    auto lock = boost::unique_lock<boost::mutex>(m_TaskLock);
    for (auto it = m_Tasks.begin(); it != m_Tasks.end(); it++)
    {
        auto pTask = std::get<0>(*it).get();
        if (pTask == task)
            return &(*it);
    }

    return NULL;
}

void CentaurusAPI::TaskAwait()
{
    auto lock = boost::unique_lock<boost::mutex>(m_TaskLock);
    m_TaskCond.wait(lock);
}

void CentaurusAPI::TaskNotify(ICentaurusTask* task)
{
    m_Notifier = task;
    m_fNotifierProgress = task->GetTaskProgress();
    m_TaskCond.notify_all();

    try {
        auto _task = ReadJSONFile(TaskFile(task));
        _task["progress"] = task->GetTaskProgress();
        _task["memoryUsage"] = task->GetMemoryUsage();
        WriteJSONFile(TaskFile(task), _task);
    } catch (const std::exception& e) {
        // nothing
    }
}

void CentaurusAPI::Idle(ICentaurusTask* task)
{
    {
        auto lock = boost::unique_lock<boost::mutex>(m_TaskLock);
        if (m_Tasks.empty()) return;
    }

    do {
        auto lock = boost::unique_lock<boost::mutex>(m_TaskLock);
        m_TaskCond.wait(lock);

        if (m_Tasks.empty())
        {
            printf("idle end\n");
            break;
        }

        ICentaurusTask* notifier = m_Notifier;
        float progress = m_fNotifierProgress;

        if (task && notifier == task)
            if (progress >= 100) break;
    } while (!m_Tasks.empty());
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
        if (std::get<0>(task)->IsBankAcquired(bank))
            return true;
    }

    return false;
}

centaurus_size CentaurusAPI::TotalMemoryUsage()
{
    auto lock = boost::unique_lock<boost::mutex>(m_TaskLock);

    centaurus_size total = 0;
    for (auto& task : m_Tasks)
        total += std::get<0>(task)->GetMemoryUsage();
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
