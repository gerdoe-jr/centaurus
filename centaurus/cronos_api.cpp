#include "cronos_api.h"
#include <crofile.h>
#include <crobank.h>

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

void CronosAPI::Invoke(ICentaurusWorker* invoker)
{
    CentaurusTask::Invoke(invoker);

    //m_BlockLimit = m_Limit / m_pBank->
    CroFile* file = m_pBank->BankFile(CROFILE_BANK);

    cronos_size blockSize = file->GetDefaultBlockSize();
    cronos_size partSize = blockSize - file->ABI()
        ->Size(cronos_first_block_hdr);

    cronos_size count = ((m_Limit / 2) / blockSize);
    m_BlockLimit = count * blockSize;
    m_ExportLimit = count * partSize;
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
    if (!AcquireBank(bank))
    {
        throw std::runtime_error("failed to load bank");
    }

    m_pBank = dynamic_cast<CentaurusBank*>(bank);
    SetLoaderFile(CROFILE_STRU);
}

CroFile* CronosAPI::SetLoaderFile(crobank_file ftype)
{
    m_pFile = m_pBank->Bank()->File(ftype);
    if (!m_pFile)
        throw std::runtime_error("SetLoaderFile no file");
    return m_pFile;
}

ICentaurusBank* CronosAPI::TargetBank() const
{
    return dynamic_cast<ICentaurusBank*>(m_pBank);
}

CroBank* CronosAPI::Bank() const
{
    return dynamic_cast<CroBank*>(m_pBank);
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
