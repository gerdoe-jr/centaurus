#include "centaurus_dbms.h"
#include "api.h"
#include "export.h"

#include <logger.h>

CENTAURUS_DBMS_API CentaurusAPI* dbms_centaurus = NULL;

CENTAURUS_DBMS_API void CentaurusDBMS_CreateAPI()
{
    dbms_centaurus = new CentaurusAPI();
    logger = dynamic_cast<ICentaurusLogger*>(dbms_centaurus);
}

CENTAURUS_DBMS_API void CentaurusDBMS_DestroyAPI()
{
    delete dbms_centaurus;
    dbms_centaurus = NULL;
    logger = NULL;
}

CENTAURUS_DBMS_API ICentaurusAPI* CentaurusDBMS_GetAPI()
{
    return dynamic_cast<ICentaurusAPI*>(dbms_centaurus);
}

CENTAURUS_DBMS_API ICentaurusTask* CentaurusDBMS_TaskExport(
    ICentaurusBank* bank)
{
    CentaurusExport* exp = new CentaurusExport();
    exp->LoadBank(bank);

    return exp;
}
