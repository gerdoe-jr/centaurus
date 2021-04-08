#ifndef __CENTAURUS_LOADER_H
#define __CENTAURUS_LOADER_H

#include "centaurus.h"
#include "task.h"
#include <crorecord.h>

class CentaurusLoader : public CentaurusTask, public ICentaurusLoader
{
public:
    CentaurusLoader();

    void RunTask() override;
    void Release() override;

    virtual void LoadBank(ICentaurusBank* bank);
    virtual CroFile* SetLoaderFile(CroBankFile ftype);
    virtual ICentaurusBank* TargetBank() const;

    CroRecordMap* GetRecordMap(unsigned id, unsigned count) override;
    CroBuffer GetRecord(unsigned id) override;
    unsigned Start() const override;
    unsigned End() const override;
    void ReleaseMap() override;
protected:
    ICentaurusBank* m_pBank;
    CroFile* m_pFile;

    CroRecordMap* m_pMap;
};

#endif