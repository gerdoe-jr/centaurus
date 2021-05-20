#include "centaurus.h"
#include <centaurus_dbms.h>

CENTAURUS_EXPORT_API bool Centaurus_Init(const std::wstring& path)
{
    try {
        CentaurusDBMS_CreateAPI();
        centaurus = CentaurusDBMS_GetAPI();
        centaurus->Init(path);
    } catch (const std::exception& e) {
        fprintf(stderr, "InitCentaurusAPI: %s\n", e.what());
        centaurus->Exit();
        
        CentaurusDBMS_DestroyAPI();
        centaurus = NULL;
    }

    return centaurus;
}

CENTAURUS_EXPORT_API void Centaurus_Exit()
{
    if (!centaurus) return;

    try {
        centaurus->Exit();
    }
    catch (const std::exception& e) {
        fprintf(stderr, "ExitCentaurusAPI: %s\n", e.what());
    }

    CentaurusDBMS_DestroyAPI();
    centaurus = NULL;
}
