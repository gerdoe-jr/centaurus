#ifndef __CROENTITY_H
#define __CROENTITY_H

#include "crotype.h"

class CroFile;

class CroEntity
{
public:
    CroEntity();
    CroEntity(CroFile* file, cronos_id id);

    inline CroFile* File() const { return m_pFile; }
    inline cronos_id Id() const { return m_Id; }
protected:
    void InitEntity(CroFile* file, cronos_id id);

    int Version() const;
    int Minor() const;

    inline bool Is3() const { return Version() == 3; }
    inline bool Is4() const { return Version() == 4; }
    inline bool Is4A() const { return Version() >= 4; }
private:
    CroFile* m_pFile;
    cronos_id m_Id;
};

#endif
