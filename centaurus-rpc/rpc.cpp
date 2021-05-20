#include "rpc.h"

/* CentaurusRPC */

class CentaurusRPC : public ICentaurusRPC
{
public:
    virtual ~CentaurusRPC() {}

    void Invoke(ICentaurusWorker* invoker) override {}
    void RunTask() override {}
    void Release() override {}
    centaurus_size GetMemoryUsage() override { return 0; }

    bool AcquireBank(ICentaurusBank* bank) override { return false; }
    CroTable* AcquireTable(CroTable&& table) override { return NULL; }
    bool IsBankAcquired(ICentaurusBank* bank) override { return false; }
    void ReleaseTable(CroTable* table) override {}

    ICentaurusWorker* Invoker() const override { return NULL; }
    const std::string& TaskName() const override { return ""; }
};

/* CentaurusRPC_Server */
 
class CentaurusRPC_Server : public CentaurusRPC
{
public:
     virtual ~CentaurusRPC_Server() {}
};

/* Centaurus RPC API */

CENTAURUS_RPC_API ICentaurusRPC* rpc = NULL;
CentaurusRPC_Server* _sv_rpc = NULL;
ICentaurusRPC* _cl_rpc = NULL;

CENTAURUS_RPC_API void CentaurusRPC_InitServer()
{
    _sv_rpc = new CentaurusRPC_Server;
}

CENTAURUS_RPC_API void CentaurusRPC_InitClient()
{
    _cl_rpc = NULL;
}

CENTAURUS_RPC_API ICentaurusRPC* CentaurusRPC_Create(std::string host, int port)
{
    if (_sv_rpc)
        return dynamic_cast<ICentaurusRPC*>(_sv_rpc);
    return dynamic_cast<ICentaurusRPC*>(_cl_rpc);
}

CENTAURUS_RPC_API void CentaurusRPC_Destroy(ICentaurusRPC* rpc)
{

}
