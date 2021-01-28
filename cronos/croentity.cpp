#include "croentity.h"
#include "crofile.h"

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

int CroEntity::Version() const
{
    return m_pFile->GetVersion();
}

int CroEntity::Minor() const
{
    return m_pFile->GetMinor();
}

