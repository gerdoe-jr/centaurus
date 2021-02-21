#include "centaurus-rpc.h"
#include <stdexcept>
#include <algorithm>
#include <typeinfo>

using namespace Centaurus;

//RPCObject* Centaurus::RPCObject::s_pFirstTable = NULL;
//RPCObject* Centaurus::RPCObject::s_pLastTable = NULL;
//
//Centaurus::RPCObject::RPCObject(const std::type_info& type,
//    std::initializer_list<rpc_method> table)
//    : m_Methods(table), m_pNextTable(NULL)
//{
//    std::string name = type.name();
//    if (name.substr(0, 5) == "class")
//        m_ObjectName = name.substr(6);
//    else m_ObjectName = "RPCObject#" + std::to_string(type.hash_code());
//    
//    if (!s_pFirstTable) s_pFirstTable = this;
//    if (s_pLastTable) s_pLastTable->m_pNextTable = this;
//    s_pLastTable = this;
//}
//
//Centaurus::RPCObject::RPCObject(rpc_id objectId)
//    : m_pNextTable(NULL)
//{
//    m_ObjectName = "RPCObject#" + std::to_string((uint64_t)this);
//}
//
//const std::string& Centaurus::RPCObject::GetObjectName() const
//{
//    return m_ObjectName;
//}
//
//void Centaurus::RPCObject::Dispatch(
//    const std::string& func, json::object& args)
//{
//    auto method = std::find_if(m_Methods.begin(), m_Methods.end(),
//        [func](const rpc_method& _method) {
//            return _method.m_MethodName == func;
//        }
//    );
//
//    if (method == m_Methods.end())
//        throw std::runtime_error("RPC dispatch " + func + " not found");
//
//    m_Call.m_FuncId = method - m_Methods.begin();
//    m_Call.m_Args = std::vector<rpc_value>();
//    for (const auto& param : method->m_Params)
//    {
//        auto& arg = args[param.second];
//        if (arg.is_null() && param.first != rpc_value_null)
//            throw std::runtime_error("no param: " + param.second);
//
//        rpc_value value;
//        switch (param.first)
//        {
//        case rpc_value_null:
//            if (arg.is_null()) value = rpc_null{};
//            else throw std::runtime_error("json param: " + param.second);
//            break;
//        case rpc_value_int:
//            if (arg.is_number())
//            {
//                if (arg.is_int64()) value = (uint64_t)arg.as_int64();
//                else if (arg.is_uint64()) value = arg.as_uint64();
//            }
//            else throw std::runtime_error("json param: " + param.second);
//            break;
//        case rpc_value_bool:
//            if (arg.is_bool()) value = arg.as_bool();
//            else throw std::runtime_error("json param: " + param.second);
//            break;
//        case rpc_value_string:
//            if (arg.is_string())
//                value = json::string_view(arg.as_string()).to_string();
//            else throw std::runtime_error("json param: " + param.second);
//            break;
//        }
//
//        m_Call.m_Args.push_back(value);
//    }
//
//    std::cout << args << std::endl;
//    method->m_Function(this);
//}

/* RPCBase */

template<typename T>
void Centaurus::RPCBase<T>::Dispatch(rpc_call& call, json::value& args)
{
    auto& method = Table()->Method(call.m_MethodId);
    if (!args.is_null())
    {
        if (!args.is_object())
            throw RPCError("Dispatch: args is not an object");

        json::object& _args = args.get_object();
        for (auto const& param : method.m_Params)
        {
            std::string name = param.second;
            if (!_args.contains(name))
                throw RPCError(method.m_MethodName + ": no arg " + name);

            json::value& arg = _args.at(name);
            switch (arg.kind())
            {
            case json::kind::null:
                if (param.first != rpc_value_null)
                    throw RPCError(method.m_MethodName + ": wrong " + name);
                call.m_Args.emplace_back(rpc_null{});
                break;
            case json::kind::bool_:
                if (param.first != rpc_value_bool)
                    throw RPCError(method.m_MethodName + ": wrong " + name);
                call.m_Args.emplace_back(arg.get_bool());
                break;
            case json::kind::int64:
                if (param.first != rpc_value_int)
                    throw RPCError(method.m_MethodName + ": wrong " + name);
                call.m_Args.emplace_back((uint64_t)arg.get_int64());
                break;
            case json::kind::uint64:
                if (param.first != rpc_value_int)
                    throw RPCError(method.m_MethodName + ": wrong " + name);
                call.m_Args.emplace_back(arg.get_uint64());
                break;
            case json::kind::string:
                if (param.first != rpc_value_string)
                    throw RPCError(method.m_MethodName + ": wrong " + name);
                call.m_Args.emplace_back(json::string_view(
                    arg.get_string()).to_string());
                break;
            default:
                throw RPCError(method.m_MethodName + ": unknown " + name);
            }
        }
    }
    else if (!method.m_Params.empty())
        throw RPCError(method.m_MethodName + ": no args");

    call.m_Return = rpc_null{};
    method(dynamic_cast<T*>(this));
}

/* RPCTable */

RPCTable* Centaurus::RPCTable::s_pFirstTable = NULL;
RPCTable* Centaurus::RPCTable::s_pLastTable = NULL;

Centaurus::RPCTable::RPCTable(const std::string& name,
    std::initializer_list<rpc_method> table)
    : m_Methods(table), m_pNextTable(NULL),
    m_TableId((rpc_id)-1), m_TableName(name)
{
    /*std::string name = type.name();
    if (name.substr(0, 5) == "class")
        m_TableName = name.substr(6);
    else m_TableName = "RPCTable#" + std::to_string(type.hash_code());*/

    if (!s_pFirstTable) s_pFirstTable = this;
    if (s_pLastTable) s_pLastTable->m_pNextTable = this;
    s_pLastTable = this;
}

RPCTable* Centaurus::RPCTable::Table()
{
    return this;
}

void Centaurus::RPCTable::Init(rpc_id tableId)
{
    m_TableId = tableId;
}

rpc_id Centaurus::RPCTable::TableId() const
{
    return m_TableId;
}

const std::string& Centaurus::RPCTable::TableName() const
{
    return m_TableName;
}

RPCTable::rpc_method& Centaurus::RPCTable::Method(rpc_id method)
{
    return m_Methods.at(method);
}

RPCTable::rpc_method& Centaurus::RPCTable::Method(const std::string& func)
{
    auto method = std::find_if(m_Methods.begin(), m_Methods.end(),
        [func](const rpc_method& _method) {
            return _method.m_MethodName == func;
        }
    );

    if (method == m_Methods.end())
        throw RPCError("table " + TableName() + " invalid method " + func);
    return *method;
}

rpc_id Centaurus::RPCTable::MethodId(const rpc_method& method) const
{
    return std::distance(m_Methods.data(), &method);
}

/* RPC */

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
    rpc_id tableId = 0;
    RPCTable* table = RPCTable::s_pFirstTable;
    if (table)
    {
        do {
            std::cout << "init " << table->TableName() << std::endl;
            table->Init(tableId++);
        } while (table = table->m_pNextTable);
    }
}

RPCTable* Centaurus::RPC::Table(rpc_id tableId)
{
    RPCTable* table = RPCTable::s_pFirstTable;
    if (table)
    {
        do {
            if (table->TableId() == tableId)
                return table;
        } while (table = table->m_pNextTable);
    }

    return NULL;
}

RPCTable* Centaurus::RPC::Table(const std::string& name)
{
    RPCTable* table = RPCTable::s_pFirstTable;
    if (table)
    {
        do {
            if (table->TableName() == name)
                return table;
        } while (table = table->m_pNextTable);
    }

    return NULL;
}

void Centaurus::RPC::Dispatch(rpc_call& call,
    rpc_id tableId, rpc_id methodId, json::object& args)
{
    RPCTable* table = Table(tableId);
    if (!table) throw RPCError("invalid table id");

    call.m_MethodId = methodId;
    json::value _args = json::value_from(args);

    table->Dispatch(call, _args);
}

/* RPCError */

Centaurus::RPCError::RPCError(const std::string& error)
    : m_Error(error)
{
}

const char* Centaurus::RPCError::what() const noexcept
{
    return m_Error.c_str();
}
