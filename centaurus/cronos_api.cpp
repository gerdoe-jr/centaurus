#include "cronos_api.h"
#include <crofile.h>

CronosAPI::CronosAPI()
    : CentaurusTask("CronosAPI")
{
    m_pBank = NULL;
    m_pFile = NULL;
    m_pMap = NULL;
}

CronosAPI::CronosAPI(const std::string& taskName)
    : CentaurusTask(taskName)
{
    m_pBank = NULL;
    m_pFile = NULL;
    m_pMap = NULL;
}

void CronosAPI::RunTask()
{
}

void CronosAPI::Release()
{
    ReleaseMap();
}

void CronosAPI::LoadBank(ICentaurusBank* bank)
{
    if (AcquireBank(bank))
    {
        m_pBank = bank;
        SetLoaderFile(CroStru);
    }
    else
    {
        throw std::runtime_error("failed to load bank");
    }
}

CroFile* CronosAPI::SetLoaderFile(CroBankFile ftype)
{
    m_pFile = m_pBank->File(ftype);
    if (!m_pFile)
        throw std::runtime_error("SetLoaderFile no file");
    return m_pFile;
}

ICentaurusBank* CronosAPI::TargetBank() const
{
    return m_pBank;
}

CroRecordMap* CronosAPI::GetRecordMap(unsigned id, unsigned count)
{
    m_pMap = AcquireTable<CroRecordMap>(m_pFile->LoadRecordMap(id, count));
    return m_pMap;
}

CroBuffer CronosAPI::GetRecord(unsigned id)
{
    if (!m_pMap->HasRecord(id))
        throw std::runtime_error("loader no record");
    return m_pMap->LoadRecord(id);
}

unsigned CronosAPI::Start() const
{
    return m_pMap->IsEmpty() ? INVALID_CRONOS_ID : m_pMap->IdStart();
}

unsigned CronosAPI::End() const
{
    return m_pMap->IsEmpty() ? INVALID_CRONOS_ID : m_pMap->IdEnd();
}

void CronosAPI::ReleaseMap()
{
    if (m_pMap)
    {
        ReleaseTable(m_pMap);
        m_pMap = NULL;
    }
}

ICentaurusLogger* CronosAPI::CronosLog()
{
    return dynamic_cast<ICentaurusLogger*>(this);
}
