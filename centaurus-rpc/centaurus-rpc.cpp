#include "centaurus-rpc.h"
#include <stdexcept>
#include <algorithm>
#include <typeinfo>

using namespace Centaurus;

RPCObject* Centaurus::RPCObject::s_pFirstTable = NULL;
RPCObject* Centaurus::RPCObject::s_pLastTable = NULL;

Centaurus::RPCObject::RPCObject(const std::type_info& type,
    std::initializer_list<rpc_method> table)
    : m_Methods(table), m_pNextTable(NULL)
{
    std::string name = type.name();
    if (name.substr(0, 5) == "class")
        m_ObjectName = name.substr(6);
    else m_ObjectName = "RPCObject#" + std::to_string(type.hash_code());
    
    if (!s_pFirstTable) s_pFirstTable = this;
    if (s_pLastTable) s_pLastTable->m_pNextTable = this;
    s_pLastTable = this;
}

Centaurus::RPCObject::RPCObject(rpc_id objectId)
    : m_pNextTable(NULL)
{
    m_ObjectName = "RPCObject#" + std::to_string((uint64_t)this);
}

const std::string& Centaurus::RPCObject::GetObjectName() const
{
    return m_ObjectName;
}

#include <iostream>

void Centaurus::RPCObject::Dispatch(
    const std::string& func, json::object& args)
{
    auto method = std::find_if(m_Methods.begin(), m_Methods.end(),
        [func](const rpc_method& _method) {
            return _method.m_MethodName == func;
        }
    );

    if (method == m_Methods.end())
        throw std::runtime_error("RPC dispatch " + func + " not found");

    m_Call.m_FuncId = method - m_Methods.begin();
    m_Call.m_Args = std::vector<rpc_value>();
    for (const auto& param : method->m_Params)
    {
        auto& arg = args[param.second];
        if (arg.is_null() && param.first != rpc_value_null)
            throw std::runtime_error("no param: " + param.second);

        rpc_value value;
        switch (param.first)
        {
        case rpc_value_null:
            if (arg.is_null()) value = rpc_null{};
            else throw std::runtime_error("json param: " + param.second);
            break;
        case rpc_value_int:
            if (arg.is_number())
            {
                if (arg.is_int64()) value = (uint64_t)arg.as_int64();
                else if (arg.is_uint64()) value = arg.as_uint64();
            }
            else throw std::runtime_error("json param: " + param.second);
            break;
        case rpc_value_bool:
            if (arg.is_bool()) value = arg.as_bool();
            else throw std::runtime_error("json param: " + param.second);
            break;
        case rpc_value_string:
            if (arg.is_string())
                value = json::string_view(arg.as_string()).to_string();
            else throw std::runtime_error("json param: " + param.second);
            break;
        }

        m_Call.m_Args.push_back(value);
    }

    std::cout << args << std::endl;
    method->m_Function(this);
}

std::shared_ptr<RPC> Centaurus::rpc;

void Centaurus::RPC::InitClient(net::ip::address address, unsigned short port)
{
    rpc = std::make_shared<RPC>(RPC_CLIENT);
    rpc->SetEndPoint(tcp::endpoint { address, port });
    rpc->Init();
}

void Centaurus::RPC::InitServer(net::ip::address address, unsigned short port)
{
    rpc = std::make_shared<RPC>(RPC_SERVER);
    rpc->SetEndPoint(tcp::endpoint{ address, port });
    rpc->Init();
}

Centaurus::RPC::RPC(RPCType type)
{
    m_Type = type;
}

void Centaurus::RPC::SetEndPoint(tcp::endpoint endpoint)
{
    m_Address = endpoint;
}

#include <iostream>

void Centaurus::RPC::Init()
{
    RPCObject* table = RPCObject::s_pFirstTable;
    if (table)
    {
        do {
            std::cout << table->GetObjectName() << std::endl;
        } while (table = table->m_pNextTable);
    }
}
