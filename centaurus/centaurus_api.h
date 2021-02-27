#ifndef __CENTAURUS_API_H
#define __CENTAURUS_API_H

#ifdef CENTAURUS_INTERNAL
#define CENTAURUS_API __declspec(dllexport)
#else
#define CENTAURUS_API __declspec(dllimport)
#endif

#endif