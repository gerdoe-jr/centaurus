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
        }
    }
});

int main()
{
    RPC::InitServer();

    json::object args{ {"first", "stringValue"}, {"second", 333} };
    
    RPCTable* table = rpc->Table("my_table");
    const RPCTable::rpc_method& method = table->Method("testfunc1");

    std::cout << "table " << table->TableName()
        << " id " << table->TableId() << std::endl;
    std::cout << "\tmethod " << method.m_MethodName << std::endl;
    std::cout << std::endl;
    return 0;
}
