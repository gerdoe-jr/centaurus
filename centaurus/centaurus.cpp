#include "centaurus.h"
#include "win32util.h"
#include "crofile.h"
#include "crobuffer.h"
#include "cronos_abi.h"
#include <functional>
#include <stdexcept>
#include <exception>
#include <memory>
#include <tuple>
#include <stdio.h>

#include <boost/filesystem.hpp>
#include <boost/atomic.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
namespace fs = boost::filesystem;
namespace sc = boost::system;

#ifdef min
#undef min
#endif

#include <algorithm>

/* Centaurus Bank */

class CCentaurusBank : public ICentaurusBank
{
public:
    virtual ~CCentaurusBank()
    {
        CroFile* stru = File(CroStru);
        CroFile* bank = File(CroBank);
        CroFile* index = File(CroIndex);

        if (stru) stru->Close();
        if (bank) bank->Close();
        if (index) index->Close();
    }

    bool LoadPath(const std::wstring& path) override
    {
        m_Path = path;

        m_Files[CroStru] = std::make_unique<CroFile>(m_Path + L"\\CroStru");
        m_Files[CroBank] = std::make_unique<CroFile>(m_Path + L"\\CroBank");
        m_Files[CroIndex] = std::make_unique<CroFile>(m_Path + L"\\CroIndex");

        for (unsigned i = 0; i < CroBankFile_Count; i++)
        {
            CroFile* file = File((CroBankFile)i);
            if (!file) continue;

            try {
                crofile_status st = file->Open();
                if (st != CROFILE_OK)
                {
                    throw std::runtime_error("CroFile status "
                        + std::to_string(st));
                }
            } catch (const std::exception& e) {
                fwprintf(stderr, L"CroFile(%s): %S\n",
                    file->GetPath().c_str(), e.what());
                m_Files[(CroBankFile)i] = NULL;
            }
        }

        return File(CroStru) || File(CroBank) || File(CroIndex);
    }

    CroFile* File(CroBankFile type) const override
    {
        return !m_Files[type] ? NULL : m_Files[type].get();
    }

    void ExportHeaders() const override
    {
        sc::error_code ec;
        std::wstring headerPath = m_Path + L"\\include";

        fs::create_directory(headerPath, ec);
        if (ec) throw std::runtime_error("ExportHeaders !create_directory");

        for (unsigned i = 0; i < CroBankFile_Count; i++)
        {
            CroFile* file = File((CroBankFile)i);
            if (!file) continue;

            FILE* fHdr = NULL;
            switch (i)
            {
            case CroStru:
                _wfopen_s(&fHdr, (headerPath + L"\\CroStru.h").c_str(), L"w");
                break;
            case CroBank:
                _wfopen_s(&fHdr, (headerPath + L"\\CroBank.h").c_str(), L"w");
                break;
            case CroIndex:
                _wfopen_s(&fHdr, (headerPath + L"\\CroIndex.h").c_str(), L"w");
                break;
            }

            if (!fHdr) throw std::runtime_error("ExportHeaders !fHdr");
            centaurus->ExportABIHeader(file->ABI(), fHdr);
            fclose(fHdr);
        }
    }
private:
    std::wstring m_Path;
    std::unique_ptr<CroFile> m_Files[CroBankFile_Count];
};

/* Centaurus Task */

class CCentaurusTask : public ICentaurusTask
{
public:
    using RunFunction = std::function<void(CCentaurusTask*)>;

    CCentaurusTask()
        : m_fTaskProgress(0)
    {
    }

    CCentaurusTask(RunFunction run)
        : m_fTaskProgress(0), m_RunFunction(run)
    {
    }

    virtual ~CCentaurusTask()
    {
    }

    virtual void StartTask()
    {
        UpdateProgress(0);

        try {
            Run();
        } catch (const boost::thread_interrupted& ti) {
            Interrupt();
        } catch (const std::exception& e) {
            fprintf(stderr, "CCentaurusTask(%p): %s\n", this, e.what());
        }

        EndTask();
    }

    virtual void EndTask()
    {
        UpdateProgress(100);
        centaurus->EndTask(this);
    }

    virtual void Interrupt()
    {
        EndTask();
    }

    virtual void Run()
    {
        if (m_RunFunction)
            m_RunFunction(this);
    }

    float GetTaskProgress() const override
    {
        return m_fTaskProgress;
    }
    
    void UpdateProgress(float progress)
    {
        m_fTaskProgress = progress;
        centaurus->TaskNotify(this);
    }

    bool AcquireBank(ICentaurusBank* bank) override
    {
        if (centaurus->IsBankAcquired(bank))
            return false;
        
        auto lock = std::unique_lock<boost::mutex>(m_DataLock);
        m_Banks.emplace_back(bank);

        return true;
    }

    CroTable* AcquireTable(CroTable&& table) override
    {
        auto lock = std::unique_lock<boost::mutex>(m_DataLock);
        return m_Tables.emplace_back(
            std::make_unique<CroTable>(std::move(table))).get();
    }

    template<typename T>
    T* AcquireTable(T&& table)
    {
        auto lock = std::unique_lock<boost::mutex>(m_DataLock);

        T* newTable = new T(std::move(table));
        m_Tables.emplace_back(std::unique_ptr<CroTable>(newTable));

        return newTable;
    }

    bool IsBankAcquired(ICentaurusBank* bank) override
    {
        auto lock = std::unique_lock<boost::mutex>(m_DataLock);
        return std::find(m_Banks.begin(), m_Banks.end(), bank)
            != m_Banks.end();
    }

    void ReleaseTable(CroTable* table) override
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

    centaurus_size GetMemoryUsage() override
    {
        auto lock = std::unique_lock<boost::mutex>(m_DataLock);

        centaurus_size total = 0;
        for (const auto& table : m_Tables)
            total += table->GetSize();
        return total;
    }
private:
    boost::atomic<float> m_fTaskProgress;
    RunFunction m_RunFunction;

    boost::mutex m_DataLock;
    std::vector<ICentaurusBank*> m_Banks;
    std::vector<std::unique_ptr<CroTable>> m_Tables;
};

/* Centaurus API */

ICentaurusBank* testbank;

class CCentaurusAPI : public ICentaurusAPI
{
public:
    using Task = std::tuple<std::unique_ptr<ICentaurusTask>, boost::thread>;

    void Init() override
    {
        m_fOutput = stdout;
        m_fError = stderr;

        SetTableSizeLimit(512 * 1024 * 1024); //512 MB
    }

    void Exit() override
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

    void SetTableSizeLimit(centaurus_size limit) override
    {
        m_TableSizeLimit = limit;
    }

    ICentaurusBank* ConnectBank(const std::wstring& path) override
    {
        auto lock = boost::unique_lock<boost::mutex>(m_BankLock);
        CCentaurusBank* bank = new CCentaurusBank();
        if (!bank->LoadPath(path))
        {
            fwprintf(m_fError, L"centaurus: failed to open bank %s\n",
                path.c_str());
            delete bank;
            return NULL;
        }

        testbank = bank;
        ICentaurusTask* test1 = new CCentaurusTask(
            [](CCentaurusTask* task) {
                task->AcquireBank(testbank);

                CroFile* data = testbank->File(CroBank);

                CroTable* table = task->AcquireTable(
                    data->LoadEntryTable(1, 50));
                printf("acquired table %p\n", table);

                printf("available table memory size %" FCroSize "\n",
                    centaurus->RequestTableSize());

                task->ReleaseTable(table);
            }
        );

        ICentaurusTask* test2 = new CCentaurusTask(
            [](CCentaurusTask* task) {
                task->AcquireBank(testbank);

                CroFile* data = testbank->File(CroBank);

                CroTable* table = task->AcquireTable(
                    data->LoadEntryTable(1, 50));
                printf("acquired table %p\n", table);
                
                printf("task2 memory usage: %" FCroSize "\n",
                    task->GetMemoryUsage());

                task->ReleaseTable(table);
            }
        );

        StartTask(test1);
        StartTask(test2);
        
        Idle();

        m_Banks.emplace_back(std::unique_ptr<CCentaurusBank>(bank));
        return bank;
    }

    void DisconnectBank(ICentaurusBank* bank) override
    {
        auto lock = boost::unique_lock<boost::mutex>(m_BankLock);
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

    void ExportABIHeader(const CronosABI* abi,
        FILE* out) const override
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

    void LogBankFiles(ICentaurusBank* bank) const override
    {
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
    }

    void LogBuffer(const CroBuffer& buf, unsigned codepage = 0) override
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
        if (!codepage) codepage = GetConsoleOutputCP();

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

            SetConsoleOutputCP(codepage);
            for (cronos_rel i = 0; i < line; i++)
            {
                if (i < len)
                {
                    uint8_t byte = buf.GetData()[off + i];
                    putc(byte >= 0x20 ? (char)byte : '.', m_fOutput);
                }
                else putc(' ', m_fOutput);
            }
            SetConsoleOutputCP(866);
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

    void StartTask(ICentaurusTask* task) override
    {
        printf("start task %p\n", task);
        auto lock = boost::unique_lock<boost::mutex>(m_TaskLock);
        m_Tasks.emplace_back(task, boost::thread(
            &ICentaurusTask::StartTask, task));
    }

    void EndTask(ICentaurusTask* task) override
    {
        printf("end task %p\n", task);
        auto lock = boost::unique_lock<boost::mutex>(m_TaskLock);
        for (auto it = m_Tasks.begin(); it != m_Tasks.end(); it++)
        {
            auto pTask = std::get<0>(*it).get();
            if (pTask == task)
            {
                m_Tasks.erase(it);
                if (m_Tasks.empty())
                    m_TaskCond.notify_all();
                return;
            }
        }

        throw std::runtime_error("centaurus->EndTask with no task");
    }

    Task* GetTask(ICentaurusTask* task)
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

    void TaskAwait() override
    {
        auto lock = boost::unique_lock<boost::mutex>(m_TaskLock);
        m_TaskCond.wait(lock);
    }

    void TaskNotify(ICentaurusTask* task) override
    {
        m_Notifier = task;
        m_fNotifierProgress = task->GetTaskProgress();
        m_TaskCond.notify_all();
    }

    void Idle(ICentaurusTask* task = NULL) override
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
            
            fprintf(m_fOutput, "task %p progress %f notify\n", notifier,
                progress);

            if (task && notifier == task)
                if (progress >= 100) break;
        } while (!m_Tasks.empty());
    }

    bool IsBankAcquired(ICentaurusBank* bank) override
    {
        auto lock = boost::unique_lock<boost::mutex>(m_TaskLock);
        for (auto& task : m_Tasks)
        {
            if (std::get<0>(task)->IsBankAcquired(bank))
                return true;
        }

        return false;
    }

    centaurus_size TotalMemoryUsage() override
    {
        auto lock = boost::unique_lock<boost::mutex>(m_TaskLock);
        
        centaurus_size total = 0;
        for (auto& task : m_Tasks)
            total += std::get<0>(task)->GetMemoryUsage();
        return total;
    }

    centaurus_size RequestTableSize() override
    {
        centaurus_size ramUsage = TotalMemoryUsage();
        if (ramUsage > m_TableSizeLimit)
            throw std::runtime_error("ramUsage > m_TableSizeLimit");
        cronos_size available = m_TableSizeLimit - ramUsage;
        return std::min(available / 4, available);
    }
private:
    FILE* m_fOutput;
    FILE* m_fError;

    centaurus_size m_TableSizeLimit;

    boost::mutex m_BankLock;
    std::vector<std::unique_ptr<CCentaurusBank>> m_Banks;

    boost::mutex m_TaskLock;
    boost::condition_variable m_TaskCond;
    boost::atomic<ICentaurusTask*> m_Notifier;
    boost::atomic<float> m_fNotifierProgress;
    std::vector<Task> m_Tasks;
};

/* Centaurus ABI */

CENTAURUS_API ICentaurusAPI* centaurus = NULL;

CENTAURUS_API bool InitCentaurusAPI()
{
    ICentaurusAPI* api = new CCentaurusAPI();
    if (!api) return false;

    try {
        api->Init();
    } catch (const std::exception& e) {
        fprintf(stderr, "InitCentaurusAPI: %s\n", e.what());
        delete api;
    }

    centaurus = api;
    return centaurus;
}

CENTAURUS_API void ExitCentaurusAPI()
{
    if (!centaurus) return;
    
    try {
        centaurus->Exit();
    } catch (const std::exception& e) {
        fprintf(stderr, "ExitCentaurusAPI: %s\n", e.what());
    }

    delete centaurus;
    centaurus = NULL;
}

/* Centaurus Export */

class CCentaurusExport : public CCentaurusTask
{
public:
    CCentaurusExport(ICentaurusBank* bank, const std::wstring& path)
        : m_pBank(bank), m_ExportPath(path)
    {
    }

    void PrepareDirs()
    {
        fs::create_directories(m_ExportPath);
    }

    void Run() override
    {
        AcquireBank(m_pBank);
        PrepareDirs();

        ExportCroFile(m_pBank->File(CroBank));
    }

    void ExportCroFile(CroFile* file)
    {
        const CronosABI* abi = file->ABI();
        file->Reset();

        cronos_id tad_table_id = 1;
        while (!file->IsEndOfEntries())
        {
            centaurus_size tad_table_size = centaurus->RequestTableSize();

            cronos_off tad_off = file->GetOffset(CRONOS_TAD)
                ? file->GetOffset(CRONOS_TAD) : abi->Offset(cronos_tad_entry);
            cronos_idx tad_count = (tad_table_size - tad_off)
                / abi->Size(cronos_tad_entry);
            
            CroEntryTable* tad = AcquireTable<CroEntryTable>(
                file->LoadEntryTable(tad_table_id, tad_count));
            if (tad->IsEmpty())
            {
                ReleaseTable(tad);
                break;
            }

            for (cronos_id id = tad->IdStart(); id != tad->IdEnd(); id++)
            {
                CroEntry entry = tad->GetEntry(id);
                if (!entry.IsActive()) continue;

                centaurus->LogBuffer(entry);
                //printf("%" FCroOff "\t%" FCroSize "\n", entry.EntryOffset(),
                //    entry.EntrySize());

                //UpdateProgress(100.0f * (float)id
                //    / (float)file->EntryCountFileSize());
            }

            tad_table_id += tad->GetEntryCount();
            ReleaseTable(tad);
        }
    }
private:
    ICentaurusBank* m_pBank;
    std::wstring m_ExportPath;
};

CENTAURUS_API ICentaurusTask* CentaurusTask_Export(ICentaurusBank* bank,
    const std::wstring& path)
{
    return new CCentaurusExport(bank, path);
}
