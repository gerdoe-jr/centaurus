#ifndef __CENTAURUS_RPC_H
#define __CENTAURUS_RPC_H

#include <stdint.h>
#include <map>
#include <queue>
#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <typeinfo>
#include <exception>
#include <functional>
#include <boost/json.hpp>
#include <boost/variant.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/thread.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/atomic.hpp>

#define CENTAURUS_RPC_PORT 3480

namespace Centaurus
{
    namespace json = boost::json;
    using rpc_id = std::size_t;

    enum rpc_value_type {
        rpc_value_null,
        rpc_value_int,
        rpc_value_bool,
        rpc_value_string
    };
    
    class rpc_null {};
    using rpc_param = std::pair<rpc_value_type, std::string>;
    using rpc_value = boost::variant<rpc_null, uint64_t, bool, std::string>;

    //RPC call with T    
    struct rpc_call {
        rpc_id m_BaseId;
        rpc_id m_MethodId;
        std::vector<rpc_value> m_Args;
        rpc_value m_Return;
    };

    class RPCTable;

    class IRPCBase
    {
    public:
        virtual void Init(rpc_id baseId) = 0;
        virtual rpc_id BaseId() const = 0;
        virtual void Dispatch(rpc_call& call) = 0;

        virtual RPCTable* Table() = 0;
    };

    template<typename T>
    class RPCBase : public IRPCBase
    {
    public:
        struct rpc_method {
            std::string m_MethodName;
            rpc_value_type m_ReturnType;
            std::vector<rpc_param> m_Params;
            std::function<void(T*)> m_Function;
        };

        void Init(rpc_id baseId) override;
        rpc_id BaseId() const override;
        void Dispatch(rpc_call& call) override;
    private:
        rpc_id m_BaseId;
    };

    class RPCTable : public RPCBase<RPCTable>
    {
    public:
        static RPCTable* s_pFirstTable;
        static RPCTable* s_pLastTable;
        RPCTable* m_pNextTable;
        
        RPCTable(const std::string& name,
            std::initializer_list<rpc_method> table);

        RPCTable* Table() override;

        void Init(rpc_id tableId);

        rpc_id TableId() const;
        const std::string& TableName() const;
        rpc_method& Method(rpc_id method);
        rpc_method& Method(const std::string& func);
        rpc_id MethodId(const rpc_method& method) const;
    private:
        std::string m_TableName;
        std::vector<rpc_method> m_Methods;
    };

    enum RPCType {
        RPC_SERVER,
        RPC_CLIENT
    };

    namespace beast = boost::beast;
    namespace http = boost::beast::http;
    namespace net = boost::asio;
    using tcp = net::ip::tcp;

    class RPCSession : public std::enable_shared_from_this<RPCSession>
    {
    public:
        RPCSession(tcp::socket socket);
    private:
        tcp::socket m_Socket;
        beast::flat_buffer m_Buffer{ 4096 };
        http::request<http::dynamic_body> m_Request;
        http::response<http::dynamic_body> m_Response;
    };

    class RPC
    {
    public:
        static void InitClient(
            net::ip::address address = net::ip::make_address_v4("127.0.0.1"),
            unsigned short port = CENTAURUS_RPC_PORT);
        static void InitServer(
            net::ip::address address = net::ip::make_address_v4("127.0.0.1"),
            unsigned short port = CENTAURUS_RPC_PORT);
        
        RPC(RPCType type);
        void SetEndPoint(tcp::endpoint address);
        
        rpc_id RegisterRPCBase(IRPCBase* base); 
        void UnregisterRPCBase(rpc_id baseId);
        IRPCBase* Base(rpc_id baseId);
        rpc_id BaseId(const IRPCBase* base) const;
        
        void InitTables();
        RPCTable* Table(rpc_id tableId);
        RPCTable* Table(const std::string& name);

        void Init();
        void Start();
        void Stop();
        void MainThread();

        void RunRPC();
        void RunIO();

        rpc_call Call(std::string table, std::string method,
            std::initializer_list<rpc_value> args);
        rpc_call Call(rpc_id base, rpc_id method, json::object& args);
        void Issue(rpc_call& call);
    protected:
        void ParseArgs(rpc_call& call, json::value& args);
        void Dispatch(rpc_call& call);
    private:
        RPCType m_Type;
        tcp::endpoint m_Address;
        
        boost::mutex m_BaseMutex;
        boost::atomic_uint64_t m_BaseCounter;
        std::map<rpc_id,IRPCBase*> m_BaseMap;

        bool m_bRunning;
        boost::thread m_RPCThread;
        boost::thread m_IOThread;

        boost::mutex m_CallMutex;
        boost::condition_variable m_Cond;
        std::queue<rpc_call> m_RPC;
    };

    extern std::shared_ptr<RPC> rpc;

    class RPCError : public std::exception
    {
    public:
        RPCError(const std::string& error);

        const char* what() const noexcept override;
    private:
        std::string m_Error;
    };

    class RPCException : public std::exception
    {
    public:
        RPCException(const rpc_call& call, const std::string& error);
        RPCException(const rpc_call& call, const std::exception& cause);

        const char* what() const noexcept override;

        const rpc_call& GetCall() const;
    private:
        rpc_call m_Call;
        std::string m_Error;
    };
}

#endif