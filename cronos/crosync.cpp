#include "crosync.h"
#include <stdexcept>
#include <win32util.h>

/* CroSync */

CroSync::CroSync()
{
}

CroSync::CroSync(cronos_size bufferSize)
{
    InitSync(bufferSize);
}

CroSync::~CroSync()
{
    Flush();
    Free();
}

void CroSync::InitSync(cronos_size bufferSize)
{
    Alloc(bufferSize);

    SetBuffer(this);
    SetPosition(0);
}

cronos_size CroSync::SyncSize() const
{
    return (cronos_size)CroStream::GetPosition();
}

bool CroSync::SyncEmpty() const
{
    return !SyncSize();
}

void CroSync::Sync(const void* src, cronos_size size)
{
    Write((uint8_t*)src, size);
}

void CroSync::Write(const uint8_t* src, cronos_size size)
{
    if (size >= Remaining())
    {
        Flush();
    }

    CroStream::Write(src, size);
}

void CroSync::Flush()
{
    SetPosition(0);
}

/* CroSyncFile */

CroSyncFile::CroSyncFile(const std::wstring& path, cronos_size bufferSize)
    : CroSync(bufferSize), m_FilePath(path)
{
}

void CroSyncFile::Flush()
{
    FILE* fSync = _wfopen(m_FilePath.c_str(), L"ab");
    if (!fSync)
    {
        throw std::runtime_error("failed to sync file");
    }

    if (!SyncEmpty())
    {
        fwrite(GetData(), SyncSize(), 1, fSync);
    }

    fclose(fSync);

    CroSync::Flush();
}

/* CroFileExport */

CroFileExport::CroFileExport(CroFile* bank, const std::wstring& filePath)
{
    m_pFile = bank;
    m_FilePath = filePath;
}

void CroFileExport::Flush()
{
    for (auto fileId : m_Files)
    {

    }
}

void CroFileExport::AddFile(cronos_id id)
{
    m_Files.emplace_back(id);
}

void CroFileExport::ExportFile(cronos_id id)
{
    
}
