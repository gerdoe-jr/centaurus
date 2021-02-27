#include <stdio.h>
#include <string>
#include <iostream>
#include "centaurus-rpc.h"

using namespace Centaurus;

#include <array>
#include <utility>

//#define declare_params(member, params) static std::vector<rpc_param> member##_params params
//
//class MyObject
//{
//public:
//    //using MemberFunction1_params = std::initializer_list<rpc_param> { { rpc_value_int, "arg" } };
//    //static std::array<rpc_param> MemberFunction1_params { { rpc_value_int, "arg" } };
//    static std::vector<rpc_param> member_params{ { rpc_value_int, "arg" } };
//    void MemberFunction1(int arg);
//};
//
//RPCTable my_table("my_table", {
//    {
//        "testfunc1", rpc_value_null, {
//            {rpc_value_string, "first"}, {rpc_value_int, "second"}
//        }, [](RPCTable* self) {
//            printf("hello world from testfunc1!\n");
//            printf("\ttable id %zu\n", self->TableId());
//            printf("\ttable name %s\n", self->TableName().c_str());
//        }
//    }
//});

int main()
{
    RPC::InitServer();
    
    rpc->Start();
    
    //std::cout << "my_table base id " << rpc->BaseId(&my_table) << std::endl;

    //rpc_call call = rpc->Call("my_table", "testfunc1", { "testValue", 333ULL });
    //rpc->Issue(call);

    boost::this_thread::sleep_for(boost::chrono::seconds(3));
    rpc->Stop();

    return 0;
}
