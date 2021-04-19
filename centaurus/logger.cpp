#include "logger.h"
#include "api.h"
#include <crofile.h>
#include <croprop.h>
#include <crorecord.h>
#include <win32util.h>
#include <cstdarg>
#include <ctime>

#ifndef WIN32
#define CP_UTF7 65000
#define CP_UTF8 65001
#else
#include <Windows.h>
#endif

#ifdef min
#undef min
#endif

#include <algorithm>

using scoped_lock = boost::mutex::scoped_lock;

CentaurusLogger::CentaurusLogger(const std::string& name)
{
    m_LogName = name;
    m_fOutput = NULL;
    m_fError = NULL;
    m_pForward = logger;
}

CentaurusLogger::CentaurusLogger(const std::string& name,
    ICentaurusLogger* forward)
{
    m_LogName = name;
    m_fOutput = NULL;
    m_fError = NULL;
    m_pForward = forward;
}

CentaurusLogger::CentaurusLogger(const std::string& name,
    FILE* out, FILE* err)
{
    m_LogName = name;
    m_fOutput = out;
    m_fError = err;
    m_pForward = NULL;
}

void CentaurusLogger::SetLogName(const std::string& name)
{
    m_LogName = name;
}

void CentaurusLogger::SetLogForward(ICentaurusLogger* forward)
{
    m_fOutput = NULL;
    m_fError = NULL;
    m_pForward = forward;
    while (m_pForward->LogForwarder())
        m_pForward = m_pForward->LogForwarder();
}

void CentaurusLogger::LockLogger()
{
    m_LogLock.lock();
}

void CentaurusLogger::UnlockLogger()
{
    m_LogLock.unlock();
}

void* CentaurusLogger::GetLogMutex()
{
    if (m_pForward) return m_pForward->GetLogMutex();

    return &m_LogLock;
}

const std::string& CentaurusLogger::GetLogName() const
{
    return m_LogName;
}

ICentaurusLogger* CentaurusLogger::LogForwarder()
{
    return m_pForward;
}

std::string CentaurusLogger::LogLevelName(CentaurusLogLevel lvl)
{
    switch (lvl)
    {
    case LogOutput: return "info";
    case LogError: return "error";
    }

    return "log";
}

void CentaurusLogger::LogWrite(CentaurusLogger* log,
    CentaurusLogLevel lvl, const char* msg,
    CentaurusLogger* from)
{
    char szTime[128] = { 0 };
    struct tm* timeinfo;
    time_t rawtime;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(szTime, 128, "%d.%m.%Y/%H:%M:%S", timeinfo);

    FILE* fLog = log->LogFile(lvl);
    std::string levelName = from->LogLevelName(lvl);
    std::string logName = from->GetLogName();
    fprintf(fLog, "[%s %s %s] %s", szTime,
        logName.c_str(), levelName.c_str(), msg);
}

void CentaurusLogger::LogPrint(CentaurusLogLevel lvl,
    const char* fmt, va_list ap)
{
    char szBuf[512] = { 0 };
    vsnprintf_s(szBuf, 512, fmt, ap);
    
    auto* log = dynamic_cast<CentaurusLogger*>
        (m_pForward ? m_pForward : this);
    
    auto lock = scoped_lock(log->GetLogBoostMutex());
    LogWrite(log, lvl, szBuf, this);
}

void CentaurusLogger::LogPrint(CentaurusLogLevel lvl, const char* fmt, ...)
{
    va_list ap;
    
    va_start(ap, fmt);
    LogPrint(lvl, fmt, ap);
    va_end(ap);
}

void CentaurusLogger::LogBankFiles(ICentaurusBank* bank)
{
    auto lock = scoped_lock(GetLogBoostMutex());
    static const char* _croName[] = { "CroStru", "CroBank", "CroIndex" };

    bank->Connect();

    for (unsigned i = 0; i < CROFILE_COUNT; i++)
    {
        CroFile* file = bank->BankFile((crobank_file)i);
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

void CentaurusLogger::LogBankStructure(ICentaurusBank* bank)
{
    if (m_pForward)
    {
        m_pForward->LogBankStructure(bank);
        return;
    }

    auto lock = scoped_lock(GetLogBoostMutex());

    fprintf(m_fOutput, "\tm_BankFormSaveVer = %u\n", bank->BankFormSaveVer());
    fprintf(m_fOutput, "\tm_BankId = %u\n", bank->BankId());
    fprintf(m_fOutput, "\tm_BankName = \"%s\"\n",
        WcharToAnsi(bank->BankName(), 866).c_str());
    fprintf(m_fOutput, "\tm_Bases\n");
    for (unsigned i = 0; i <= bank->BaseEnd(); i++)
    {
        auto& base = bank->Base(i);
        fprintf(m_fOutput, "\t\t%03u\t\"%s\"\n", base.m_BaseIndex,
            WcharToAnsi(TextToWchar(base.m_Name), 866).c_str());
    }
    /*fprintf(m_fOutput, "\tm_Formuls\n");
    for (auto& formula : m_Formuls)
    {
        fprintf(m_fOutput, "\t\t\"%s\"\n", GetString(formula.GetData(),
            formula.GetSize()).c_str());
    }*/
    fprintf(m_fOutput, "\tm_BankSerial = %u\n", bank->BankSerial());
    fprintf(m_fOutput, "\tm_BankCustomProt = %u\n", bank->BankCustomProt());
    fprintf(m_fOutput, "\tm_BankSysPass = \"%s\"\n", WcharToAnsi(
        bank->BankSysPass(), 866).c_str());
    fprintf(m_fOutput, "\tm_BankVersion = %d\n", bank->BankVersion());
}

void CentaurusLogger::LogBuffer(const CroBuffer& buf, unsigned codepage)
{
    if (m_pForward)
    {
        m_pForward->LogBuffer(buf, codepage);
        return;
    }

    auto lock = scoped_lock(GetLogBoostMutex());

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

void CentaurusLogger::LogTable(const CroTable& table)
{
    Log("%s table 0x%" FCroOff " %" FCroSize " ID "
        "%" FCroId "-%" FCroId " count %" FCroIdx "\n",
        table.GetFileType() == CRONOS_TAD ? "TAD" : "DAT",
        table.TableOffset(), table.TableSize(),
        table.IdStart(), table.IdEnd(),
        table.GetEntryCount()
    );
}

void CentaurusLogger::LogRecordMap(const CroRecordMap& records)
{
    if (m_pForward)
    {
        m_pForward->LogRecordMap(records);
        return;
    }

    Log("%s record map 0x%" FCroOff " %" FCroSize " ID "
        "%" FCroId "-%" FCroId " count %" FCroIdx "\n",
        records.GetFileType() == CRONOS_TAD ? "TAD" : "DAT",
        records.TableOffset(), records.TableSize(),
        records.IdStart(), records.IdEnd(),
        records.GetEntryCount()
    );

    FILE* fOut = LogFile(LogOutput);
    for (auto& [id, record] : records.PartMap())
    {
        auto& parts = record.RecordParts();
        fprintf(fOut, "  [%" FCroId "] ", id);

        for (auto it = parts.begin(); it != parts.end(); it++)
        {
            fprintf(fOut, "0x%" FCroOff " %" FCroSize "",
                it->m_PartOff, it->m_PartSize);
            fprintf(fOut, std::next(it) == parts.end() ? "\n" : " -> ");
        }
    }
}

void CentaurusLogger::Log(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    LogPrint(LogOutput, fmt, ap);
    va_end(ap);
}

void CentaurusLogger::Error(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    LogPrint(LogError, fmt, ap);
    va_end(ap);
}

FILE* CentaurusLogger::LogFile(CentaurusLogLevel lvl)
{
    FILE* fLog = NULL;
    switch (lvl)
    {
    case LogOutput: fLog = m_fOutput; break;
    case LogError: fLog = m_fError; break;
    default: 
        throw std::runtime_error("log file invalid level");
    }

    if (!fLog)
    {
        throw std::runtime_error("no log file");
    }

    return fLog;
}
