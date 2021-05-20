#ifndef __CENTAURUS_RPC_H
#define __CENTAURUS_RPC_H

#include <centaurus_api.h>

#ifdef WIN32
#ifdef CENTAURUS_RPC
#define CENTAURUS_RPC_API __declspec(dllexport)
#else
#define CENTAURUS_RPC_API __declspec(dllimport)
#endif
#else
#define CENTAURUS_RPC_API
#endif

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

extern CENTAURUS_RPC_API ICentaurusRPC* rpc;

CENTAURUS_RPC_API void CentaurusRPC_InitServer();
CENTAURUS_RPC_API void CentaurusRPC_InitClient();

CENTAURUS_RPC_API ICentaurusRPC* CentaurusRPC_Create(std::string host, int port);
CENTAURUS_RPC_API void CentaurusRPC_Destroy(ICentaurusRPC* rpc);

#endif