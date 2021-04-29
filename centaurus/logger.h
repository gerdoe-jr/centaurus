#ifndef __CENTAURUS_LOGGER_H
#define __CENTAURUS_LOGGER_H

#include "centaurus.h"

#include <cstdarg>
#include <boost/thread/mutex.hpp>

class CentaurusLogger : public ICentaurusLogger
{
public:
    CentaurusLogger(const std::string& name);
    CentaurusLogger(const std::string& name, ICentaurusLogger* forward);
    CentaurusLogger(const std::string& name, FILE* out, FILE* err);
    virtual ~CentaurusLogger();

    void SetLogName(const std::string& name);
    void SetLogForward(ICentaurusLogger* forward);

    void OpenLog(const std::wstring& outputLog,
        const std::wstring& errorLog) override;
    void SetLogIO() override;
    bool IsLogOpen() const override;
    void CloseLog() override;

    void LockLogger() override;
    void UnlockLogger() override;
    void* GetLogMutex() override;
    const std::string& GetLogName() const override;
    ICentaurusLogger* LogForwarder() override;

    std::string LogLevelName(CentaurusLogLevel lvl);
    void LogWrite(CentaurusLogger* log, CentaurusLogLevel lvl,
        const char* msg, CentaurusLogger* from);

    void LogPrint(CentaurusLogLevel lvl,
        const char* fmt, va_list ap) override;
    void LogPrint(CentaurusLogLevel lvl,
        const char* fmt, ...) override;

    void LogBankFiles(ICentaurusBank* bank) override;
    void LogBankStructure(ICentaurusBank* bank) override;
    void LogBuffer(const CroBuffer& buf, unsigned codepage = 0) override;
    void LogTable(const CroTable& table) override;
    void LogRecordMap(const CroRecordMap& records) override;

    void Log(const char* fmt, ...) override;
    void Error(const char* fmt, ...) override;
protected:
    FILE* LogFile(CentaurusLogLevel lvl);

    inline boost::mutex& GetLogBoostMutex()
    {
        return *(boost::mutex*)GetLogMutex();
    }
private:
    boost::mutex m_LogLock;
    std::string m_LogName;
    
    FILE* m_fOutput;
    FILE* m_fError;

    ICentaurusLogger* m_pForward;
};

#endif