#ifndef __CRONOS_API_H
#define __CRONOS_API_H

#include "centaurus.h"
#include "task.h"
#include <crorecord.h>

class CronosAPI : public CentaurusTask, public ICronosAPI
{
public:
    CronosAPI();
    CronosAPI(const std::string& taskName);

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

    ICentaurusLogger* CronosLog() override;
protected:
    ICentaurusBank* m_pBank;
    CroFile* m_pFile;

    CroRecordMap* m_pMap;
};

#endif