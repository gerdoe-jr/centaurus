#include "centaurus.h"
#include "logger.h"
#include "api.h"
#include "task.h"
#include "export.h"

CENTAURUS_API ICentaurusAPI* centaurus = NULL;
CENTAURUS_API ICentaurusLogger* logger = NULL;

CENTAURUS_API bool Centaurus_Init(const std::wstring& path)
{
    _centaurus = new CentaurusAPI();
    try {
        centaurus = _centaurus;
        _centaurus->Init(path);

        logger = dynamic_cast<ICentaurusLogger*>(_centaurus);
    } catch (const std::exception& e) {
        fprintf(stderr, "InitCentaurusAPI: %s\n", e.what());
        _centaurus->Exit();
        
        delete _centaurus;
        centaurus = NULL;
        logger = NULL;
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
    logger = NULL;
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

CENTAURUS_API ICentaurusLogger* CentaurusLogger_Create(std::string name,
    FILE* fOut, FILE* fErr)
{
    return new CentaurusLogger(name, fOut, fErr);
}

CENTAURUS_API ICentaurusLogger* CentaurusLogger_Forward(std::string name,
    ICentaurusLogger* forward)
{
    return new CentaurusLogger(name, forward);
}

CENTAURUS_API void CentaurusLogger_Destroy(ICentaurusLogger* log)
{
    auto* _log = dynamic_cast<CentaurusLogger*>(log);
    if (!_log) throw std::runtime_error("invalid logger");

    delete _log;
}
