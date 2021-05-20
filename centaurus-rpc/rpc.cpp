#include "rpc.h"

/* CentaurusRPC */

class CentaurusRPC : public ICentaurusRPC
{
public:
    virtual ~CentaurusRPC() {}

    void Invoke(ICentaurusWorker* invoker) override;
    void RunTask() override;
    void Release() override;
    centaurus_size GetMemoryUsage() override;

    bool AcquireBank(ICentaurusBank* bank) override;
    CroTable* AcquireTable(CroTable&& table) override;
    bool IsBankAcquired(ICentaurusBank* bank) override;
    void ReleaseTable(CroTable* table) override;

    ICentaurusWorker* Invoker() const override;
    const std::string& TaskName() const override;
};

/* CentaurusRPC_Server */
 
class CentaurusRPC_Server : public CentaurusRPC
{
public:
     virtual ~CentaurusRPC_Server() {}
};

/* Centaurus RPC API */

CENTAURUS_API ICentaurusRPC* rpc = NULL;
CentaurusRPC_Server* _sv_rpc = NULL;
ICentaurusRPC* _cl_rpc = NULL;

CENTAURUS_API void CentaurusRPC_InitServer()
{
    _sv_rpc = new CentaurusRPC_Server;
}

CENTAURUS_API void CentaurusRPC_InitClient()
{
    _cl_rpc = NULL;
}

CENTAURUS_API ICentaurusRPC* CentaurusRPC_Create(std::string host, int port)
{
    if (_sv_rpc)
        return dynamic_cast<ICentaurusRPC*>(_sv_rpc);
    return dynamic_cast<ICentaurusRPC*>(_cl_rpc);
}

CENTAURUS_API void CentaurusRPC_Destroy(ICentaurusRPC* rpc)
{

}
