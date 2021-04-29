#ifndef __CRONOS_API_H
#define __CRONOS_API_H

#include "centaurus.h"
#include "bank.h"
#include "task.h"
#include <crorecord.h>
#include <crosync.h>

class CronosAPI : public CentaurusTask, public ICronosAPI
{
public:
    CronosAPI();
    CronosAPI(const std::string& taskName);

    virtual void Invoke(ICentaurusWorker* invoker = NULL);
    
    void RunTask() override;
    void Release() override;

    virtual void LoadBank(ICentaurusBank* bank);
    virtual CroFile* SetLoaderFile(crobank_file ftype);
    virtual ICentaurusBank* TargetBank() const override;
    virtual CroBank* Bank() const override;

    CroRecordMap* GetRecordMap(unsigned id, unsigned count) override;
    CroBuffer GetRecord(unsigned id) override;
    unsigned Start() const override;
    unsigned End() const override;
    void ReleaseMap() override;

    ICentaurusLogger* CronosLog() override;

    template<typename T> inline CroSync* AllocBuffer(cronos_size bufferSize)
    {
        T* buffer = new T();

        buffer->InitSync(bufferSize);
        AcquireBuffer(buffer);

        return dynamic_cast<CroSync*>(buffer);
    }

    CroSync* CreateSyncFile(const std::wstring& path, cronos_size bufferSize);
    void FlushBuffers();
    void ReleaseBuffers();
protected:
    void AcquireBuffer(CroSync* sync);

    CentaurusBank* m_pBank;
    CroFile* m_pFile;
    CroRecordMap* m_pMap;

    centaurus_size m_BlockLimit;
    centaurus_size m_ExportLimit;
private:
    std::vector<std::unique_ptr<CroSync>> m_Buffers;
};

#endif