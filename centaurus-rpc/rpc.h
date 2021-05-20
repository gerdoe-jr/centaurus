#ifndef __CENTAURUS_RPC_H
#define __CENTAURUS_RPC_H

#include "centaurus.h"

class ICentaurusRPC : public ICentaurusTask
{
public:
    virtual ~ICentaurusRPC() {}

    virtual void Invoke(ICentaurusWorker* invoker) = 0;
    virtual void RunTask() = 0;
    virtual void Release() = 0;
    virtual centaurus_size GetMemoryUsage() = 0;

    virtual bool AcquireBank(ICentaurusBank* bank) = 0;
    virtual CroTable* AcquireTable(CroTable&& table) = 0;
    virtual bool IsBankAcquired(ICentaurusBank* bank) = 0;
    virtual void ReleaseTable(CroTable* table) = 0;

    virtual ICentaurusWorker* Invoker() const = 0;
    virtual const std::string& TaskName() const = 0;
};

extern CENTAURUS_API ICentaurusRPC* rpc;

CENTAURUS_API void CentaurusRPC_InitServer();
CENTAURUS_API void CentaurusRPC_InitClient();

CENTAURUS_API ICentaurusRPC* CentaurusRPC_Create(std::string host, int port);
CENTAURUS_API void CentaurusRPC_Destroy(ICentaurusRPC* rpc);

#endif