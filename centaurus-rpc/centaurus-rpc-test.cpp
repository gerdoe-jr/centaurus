#include <stdio.h>
#include <string>
#include <iostream>
#include "centaurus-rpc.h"

using namespace Centaurus;

RPCTable my_table("my_table", {
    {
        "testfunc1", rpc_value_null, {
            {rpc_value_string, "first"}, {rpc_value_int, "second"}
        }, [](RPCTable* self) {
            printf("hello world from testfunc1!\n");
            printf("\ttable id %zu\n", self->TableId());
            printf("\ttable name %s\n", self->TableName().c_str());
        }
    }
});

int main()
{
    RPC::InitServer();

    json::object args{ {"first", "stringValue"}, {"second", 333} };
    
    rpc_call call;
    rpc->Dispatch(call, my_table.TableId(), 0, args);

    return 0;
}
