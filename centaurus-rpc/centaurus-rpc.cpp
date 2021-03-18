#include "centaurus-rpc.h"
#include <stdexcept>
#include <algorithm>
#include <typeinfo>

using namespace Centaurus;

/* RPCObject */



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

    m_ObjectCounter = 0;
}

void Centaurus::RPC::SetEndPoint(tcp::endpoint endpoint)
{
    m_Address = endpoint;
}



void Centaurus::RPC::Init()
{
    //InitTables();
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

            //Dispatch(call);
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
    /*IRPCBase* base = Base(call.m_BaseId);
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
        throw RPCError(method.m_MethodName + ": no args");*/
}

void Centaurus::RPC::Dispatch(rpc_call& call)
{
    /*IRPCBase* base = Base(call.m_BaseId);
    if (!base) throw RPCError("dispatch invalid base id");
    
    std::cout << "thread dispatch " << boost::this_thread::get_id()
        << " base " << call.m_BaseId
        << " method " << call.m_MethodId
        << std::endl;
    base->Dispatch(call);*/
}

rpc_call Centaurus::RPC::Call(std::string table, std::string method,
    std::initializer_list<rpc_value> args)
{
    /*auto* _table = Table(table);
    if (!_table) throw RPCError("call invalid table");

    auto& _method = _table->Method(method);
    return rpc_call {
        _table->TableId(),
        _table->MethodId(_method),
        args,
        rpc_null{}
    };*/
}

rpc_call Centaurus::RPC::Call(rpc_id base, rpc_id method,
    json::object& args)
{
    /*rpc_call call;

    call.m_BaseId = base;
    call.m_MethodId = method;

    json::value _args = json::value_from(args);
    ParseArgs(call, _args);

    return call;*/
}

void Centaurus::RPC::Issue(rpc_call& call)
{
    /*boost::unique_lock<boost::mutex> lock(m_CallMutex);

    m_RPC.push(call);
    m_Cond.notify_one();
    std::cout << "thread issue " << boost::this_thread::get_id()
        << " base " << call.m_BaseId
        << " method " << call.m_MethodId
        << std::endl;*/
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
