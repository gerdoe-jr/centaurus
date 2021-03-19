#include "centaurus.h"
#include "centaurus_api.h"
#include "centaurus_task.h"
#include "centaurus_export.h"

CENTAURUS_API ICentaurusAPI* centaurus = NULL;

CENTAURUS_API bool Centaurus_Init(const std::wstring& path)
{
    ICentaurusAPI* api = new CentaurusAPI();
    if (!api) return false;

    try {
        centaurus = api;
        api->Init(path);
    } catch (const std::exception& e) {
        fprintf(stderr, "InitCentaurusAPI: %s\n", e.what());
        api->Exit();
        
        centaurus = NULL;
        delete api;
    }

    return centaurus;
}

CENTAURUS_API void Centaurus_Exit()
{
    if (!centaurus) return;

    try {
        centaurus->Exit();
    }
    catch (const std::exception& e) {
        fprintf(stderr, "ExitCentaurusAPI: %s\n", e.what());
    }

    delete centaurus;
    centaurus = NULL;
}

#include <boost/thread.hpp>

static boost::thread _centaurus_thread;

CENTAURUS_API void Centaurus_RunThread()
{
    _centaurus_thread = boost::thread([]() { centaurus->Run(); });
}

CENTAURUS_API void Centaurus_Idle()
{
    _centaurus_thread.join();
}

CENTAURUS_API ICentaurusTask* CentaurusTask_Export(ICentaurusBank* bank)
{
    CentaurusExport* exp = new CentaurusExport();
    exp->SetTargetBank(bank);

    return exp;
}
