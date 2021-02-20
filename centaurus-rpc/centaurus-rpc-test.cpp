#include <stdio.h>
#include <string>
#include "centaurus-rpc.h"

using namespace Centaurus;

class MyAPIObject
{
public:
    //RPCFunction testfunc1{ typeid(MyAPIObject) };
    void TestFunction1(std::string first, int second);
};

RPCObject rpc_myapiobject(typeid(MyAPIObject), std::initializer_list<rpc_method> {
    {
        "testFunction1", rpc_value_null,
        {
            std::make_pair(rpc_value_string, "first"),
            std::make_pair(rpc_value_int, "second")
        },
        
        [](RPCObject* obj) {
            printf("printf from testFunction1 !\n");
            return 23;
        }
    }
});

int main()
{
    RPC::InitServer();

    json::object args{ {"first", "stringValue"}, {"second", 333} };
    rpc_myapiobject.Dispatch("testFunction1", args);
    return 0;
}
