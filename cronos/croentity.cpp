#include "croentity.h"
#include "crofile.h"
#include "cronos_abi.h"

CroEntity::CroEntity()
{
    InitEntity(NULL, INVALID_CRONOS_ID);
}

CroEntity::CroEntity(CroFile* file, cronos_id id)
{
    InitEntity(file, id);
}

void CroEntity::InitEntity(CroFile* file, cronos_id id)
{
    m_pFile = file;
    m_Id = id;
}

const CronosABI* CroEntity::ABI() const
{
    return m_pFile->ABI();
}

int CroEntity::Version() const
{
    return ABI()->GetVersion();
}

int CroEntity::Minor() const
{
    return ABI()->GetABIVersion().second;
}

