#include "centaurus-rpc.h"
#include <stdexcept>
#include <algorithm>
#include <typeinfo>

using namespace Centaurus;

/* RPCBase */

template<typename T>
void Centaurus::RPCBase<T>::Init(rpc_id baseId)
{
    m_BaseId = baseId;
}

template<typename T>
rpc_id Centaurus::RPCBase<T>::BaseId() const
{
    return m_BaseId;
}

template<typename T>
void Centaurus::RPCBase<T>::Dispatch(rpc_call& call)
{
    const auto& method = Table()->Method(call.m_MethodId);

    call.m_Return = rpc_null{};
    method.m_Function(dynamic_cast<T*>(this));
}

/* RPCTable */

RPCTable* Centaurus::RPCTable::s_pFirstTable = NULL;
RPCTable* Centaurus::RPCTable::s_pLastTable = NULL;

Centaurus::RPCTable::RPCTable(const std::string& name,
    std::initializer_list<rpc_method> table)
    : m_Methods(table), m_pNextTable(NULL),
    m_TableName(name)
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
    RPCBase::Init(tableId);
}

rpc_id Centaurus::RPCTable::TableId() const
{
    return BaseId();
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
    m_bRunning = false;

    m_BaseCounter = 0;
}

void Centaurus::RPC::SetEndPoint(tcp::endpoint endpoint)
{
    m_Address = endpoint;
}

rpc_id Centaurus::RPC::RegisterRPCBase(IRPCBase* base)
{
    boost::unique_lock<boost::mutex> lock(m_BaseMutex);
    
    rpc_id baseId = m_BaseCounter++;
    m_BaseMap.insert(std::make_pair(baseId, base));

    base->Init(baseId);
    return base->BaseId();
}

void Centaurus::RPC::UnregisterRPCBase(rpc_id baseId)
{
    boost::unique_lock<boost::mutex> lock(m_BaseMutex);
    auto it = m_BaseMap.find(baseId);
    if (it == m_BaseMap.end())
        throw RPCError("invalid base id");
    m_BaseMap.erase(it);
}

IRPCBase* Centaurus::RPC::Base(rpc_id baseId)
{
    boost::unique_lock<boost::mutex> lock(m_BaseMutex);
    auto it = m_BaseMap.find(baseId);
    if (it == m_BaseMap.end())
        return NULL;
    return it->second;
}

rpc_id Centaurus::RPC::BaseId(const IRPCBase* base) const
{
    for (const auto& reg : m_BaseMap)
        if (reg.second == base) return reg.first;
    return -1;
}

void Centaurus::RPC::InitTables()
{
    RPCTable* table = RPCTable::s_pFirstTable;
    if (table)
    {
        do {
            RegisterRPCBase(table);
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

void Centaurus::RPC::Init()
{
    InitTables();
}

void Centaurus::RPC::Start()
{
    m_bRunning = true;

    m_RPCThread = boost::thread(&RPC::RunRPC, this);
    m_IOThread = boost::thread(&RPC::RunIO, this);
}

void Centaurus::RPC::Stop()
{
    m_bRunning = false;

    m_RPCThread.interrupt();
    m_IOThread.interrupt();

    MainThread();
}

void Centaurus::RPC::MainThread()
{
    m_RPCThread.join();
    m_IOThread.join();
}

#include <iostream>

void Centaurus::RPC::RunRPC()
{
    while (m_bRunning)
    {
        try {
            boost::unique_lock<boost::mutex> lock(m_CallMutex);

            while (m_RPC.empty())
                m_Cond.wait(lock);
            rpc_call call = m_RPC.front();
            m_RPC.pop();

            Dispatch(call);
        } catch (const boost::thread_interrupted& i) {
            //free all RPC objects
            break;
        } catch (const std::exception& e) {
            std::cout << "RPC: " << e.what() << std::endl;
        }
    }
}

void Centaurus::RPC::RunIO()
{
    while (m_bRunning)
    {
        try {
            boost::this_thread::sleep_for(boost::chrono::seconds(1));
        } catch (const boost::thread_interrupted& i) {
            //interrupt all RPC sessions
            break;
        } catch (const std::exception& e) {
            std::cout << "IO: " << e.what() << std::endl;
        }
    }
}

void Centaurus::RPC::ParseArgs(rpc_call& call, json::value& args)
{
    IRPCBase* base = Base(call.m_BaseId);
    auto& method = base->Table()->Method(call.m_MethodId);
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
}

void Centaurus::RPC::Dispatch(rpc_call& call)
{
    IRPCBase* base = Base(call.m_BaseId);
    if (!base) throw RPCError("dispatch invalid base id");
    
    std::cout << "thread dispatch " << boost::this_thread::get_id()
        << " base " << call.m_BaseId
        << " method " << call.m_MethodId
        << std::endl;
    base->Dispatch(call);
}

rpc_call Centaurus::RPC::Call(std::string table, std::string method,
    std::initializer_list<rpc_value> args)
{
    auto* _table = Table(table);
    if (!_table) throw RPCError("call invalid table");

    auto& _method = _table->Method(method);
    return rpc_call {
        _table->TableId(),
        _table->MethodId(_method),
        args,
        rpc_null{}
    };
}

rpc_call Centaurus::RPC::Call(rpc_id base, rpc_id method,
    json::object& args)
{
    rpc_call call;

    call.m_BaseId = base;
    call.m_MethodId = method;

    json::value _args = json::value_from(args);
    ParseArgs(call, _args);

    return call;
}

void Centaurus::RPC::Issue(rpc_call& call)
{
    boost::unique_lock<boost::mutex> lock(m_CallMutex);

    m_RPC.push(call);
    m_Cond.notify_one();
    std::cout << "thread issue " << boost::this_thread::get_id()
        << " base " << call.m_BaseId
        << " method " << call.m_MethodId
        << std::endl;
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

/* RPCException */

Centaurus::RPCException::RPCException(const rpc_call& call,
    const std::string& error)
    : m_Call(call), m_Error(error)
{
}

Centaurus::RPCException::RPCException(const rpc_call& call,
    const std::exception& cause)
    : m_Call(call), m_Error(cause.what())
{
}

const char* Centaurus::RPCException::what() const noexcept
{
    return m_Error.c_str();
}

const rpc_call& Centaurus::RPCException::GetCall() const
{
    return m_Call;
}
