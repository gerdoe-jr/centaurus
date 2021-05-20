#ifndef __CROSYNC_H
#define __CROSYNC_H

#include "crobuffer.h"
#include "crostream.h"
#include <string>

class CroSync : protected CroBuffer, protected CroStream
{
public:
    CroSync();
    CroSync(cronos_size bufferSize);
    virtual ~CroSync();

    void InitSync(cronos_size bufferSize);
    cronos_size SyncSize() const;
    bool SyncEmpty() const;

    virtual void Sync(const void* src, cronos_size size);
    template<typename T> inline void Sync(const T& val)
    {
        Sync(&val, sizeof(val));
    }

    virtual void Write(const uint8_t* src, cronos_size size);
    virtual void Flush();
};

class CroSyncFile : public CroSync
{
public:
    CroSyncFile(const std::wstring& path, cronos_size bufferSize);

    void Flush() override;
private:
    std::wstring m_FilePath;
};

class ICroFileExport
{
public:
    virtual void AddFile(cronos_id id) = 0;
    virtual void ExportFile(cronos_id id) = 0;
};

class CroFileExport : public CroSync, ICroFileExport
{
public:
    CroFileExport(CroFile* bank, const std::wstring& filePath);

    void Flush() override;

    void AddFile(cronos_id id) override;
    void ExportFile(cronos_id id) override;
private:
    CroFile* m_pFile;
    std::wstring m_FilePath;
    std::vector<cronos_id> m_Files;
};

#endif