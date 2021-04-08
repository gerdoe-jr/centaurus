#include "loader.h"
#include <crofile.h>

CentaurusLoader::CentaurusLoader()
{
    m_pBank = NULL;
    m_pFile = NULL;
    m_pMap = NULL;
}

void CentaurusLoader::RunTask()
{
}

void CentaurusLoader::Release()
{
    ReleaseMap();
}

void CentaurusLoader::LoadBank(ICentaurusBank* bank)
{
    m_pBank = bank;
    SetLoaderFile(CroStru);
}

CroFile* CentaurusLoader::SetLoaderFile(CroBankFile ftype)
{
    m_pFile = m_pBank->File(ftype);
    if (!m_pFile)
        throw std::runtime_error("SetLoaderFile no file");
    return m_pFile;
}

ICentaurusBank* CentaurusLoader::TargetBank() const
{
    return m_pBank;
}

CroRecordMap* CentaurusLoader::GetRecordMap(unsigned id, unsigned count)
{
    m_pMap = AcquireTable<CroRecordMap>(m_pFile->LoadRecordMap(id, count));
    return m_pMap;
}

CroBuffer CentaurusLoader::GetRecord(unsigned id)
{
    if (!m_pMap->HasRecord(id))
        throw std::runtime_error("loader no record");
    return m_pMap->LoadRecord(id);
}

unsigned CentaurusLoader::Start() const
{
    return m_pMap->IsEmpty() ? INVALID_CRONOS_ID : m_pMap->IdStart();
}

unsigned CentaurusLoader::End() const
{
    return m_pMap->IsEmpty() ? INVALID_CRONOS_ID : m_pMap->IdEnd();
}

void CentaurusLoader::ReleaseMap()
{
    if (m_pMap)
    {
        ReleaseTable(m_pMap);
        m_pMap = NULL;
    }
}
