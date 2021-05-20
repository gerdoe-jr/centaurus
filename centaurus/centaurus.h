#ifndef __CENTAURUS_H
#define __CENTAURUS_H

#include <centaurus_api.h>

#ifdef WIN32
#ifdef CENTAURUS_DLL
#define CENTAURUS_EXPORT_API __declspec(dllexport)
#else
#define CENTAURUS_EXPORT_API __declspec(dllimport)
#endif
#else
#define CENTAURUS_EXPORT_API
#endif

CENTAURUS_EXPORT_API bool Centaurus_Init(const std::wstring& path);
CENTAURUS_EXPORT_API void Centaurus_Exit();

#endif