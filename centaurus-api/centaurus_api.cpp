#include "centaurus_api.h"
#include <boost/thread.hpp>

CENTAURUS_API ICentaurusAPI* centaurus = NULL;

static boost::thread _centaurus_thread;

CENTAURUS_API void CentaurusAPI_RunThread()
{
    _centaurus_thread = boost::thread([]() { centaurus->Run(); });
}

CENTAURUS_API void CentaurusAPI_Idle()
{
    _centaurus_thread.join();
}
