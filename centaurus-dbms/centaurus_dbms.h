#ifndef __CENTAURUS_DBMS_H
#define __CENTAURUS_DBMS_H

#include <centaurus_api.h>

class CentaurusAPI;

#ifdef WIN32
#ifdef CENTAURUS_DBMS
#define CENTAURUS_DBMS_API __declspec(dllexport)
#else
#define CENTAURUS_DBMS_API __declspec(dllimport)
#endif
#else
#define CENTAURUS_DBMS_API
#endif

extern CENTAURUS_DBMS_API CentaurusAPI* dbms_centaurus;
#define _centaurus dbms_centaurus

CENTAURUS_DBMS_API void CentaurusDBMS_CreateAPI();
CENTAURUS_DBMS_API void CentaurusDBMS_DestroyAPI();
CENTAURUS_DBMS_API ICentaurusAPI* CentaurusDBMS_GetAPI();

//CENTAURUS_DBMS_API ICentaurusTask* CentaurusTask_Run(CentaurusRun run);
CENTAURUS_DBMS_API ICentaurusTask* CentaurusDBMS_TaskExport(ICentaurusBank* bank);

#endif