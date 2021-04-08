#include "centaurus.h"
#include "api.h"
#include "task.h"
#include "export.h"

CENTAURUS_API ICentaurusAPI* centaurus = NULL;

CENTAURUS_API bool Centaurus_Init(const std::wstring& path)
{
    _centaurus = new CentaurusAPI();
    try {
        centaurus = _centaurus;
        _centaurus->Init(path);
    } catch (const std::exception& e) {
        fprintf(stderr, "InitCentaurusAPI: %s\n", e.what());
        _centaurus->Exit();
        
        delete _centaurus;
        centaurus = NULL;
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

    delete _centaurus;
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
    exp->LoadBank(bank);

    return exp;
}
