#include "centaurus.h"
#include "centaurus_api.h"
#include "centaurus_task.h"
#include "centaurus_export.h"

CENTAURUS_API ICentaurusAPI* centaurus = NULL;

CENTAURUS_API bool Centaurus_Init()
{
    ICentaurusAPI* api = new CentaurusAPI();
    if (!api) return false;

    try {
        api->Init();
    }
    catch (const std::exception& e) {
        fprintf(stderr, "InitCentaurusAPI: %s\n", e.what());
        delete api;
    }

    centaurus = api;
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

CENTAURUS_API ICentaurusTask* CentaurusTask_Run(CentaurusRun run)
{
    return new CentaurusTask(run);
}

CENTAURUS_API ICentaurusTask* CentaurusTask_Export(ICentaurusBank* bank,
    const std::wstring& path)
{
    return new CentaurusExport(bank, path);
}
