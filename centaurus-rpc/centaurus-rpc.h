#ifndef __CENTAURUS_RPC_H
#define __CENTAURUS_RPC_H

#include <stdint.h>
#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <typeinfo>
#include <functional>
#include <boost/json.hpp>
#include <boost/thread.hpp>
#include <boost/variant.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#define CENTAURUS_RPC_PORT 3480

namespace Centaurus
{
    namespace json = boost::json;
    using rpc_id = unsigned;

    enum rpc_value_type {
        rpc_value_null,
        rpc_value_int,
        rpc_value_bool,
        rpc_value_string
    };
    
    class rpc_null {};
    using rpc_param = std::pair<rpc_value_type, std::string>;
    using rpc_value = boost::variant<rpc_null, uint64_t, bool, std::string>;
    
    class RPCObject;
    using rpc_function = std::function<void(RPCObject*)>;

    struct rpc_method {
        std::string m_MethodName;
        rpc_value_type m_Return;
        std::vector<rpc_param> m_Params;
        rpc_function m_Function;
    };

    struct rpc_call {
        rpc_id m_FuncId;
        std::vector<rpc_value> m_Args;
        rpc_value m_Return;
    };
    
    class RPCObject
    {
    public:
        static RPCObject* s_pFirstTable;
        static RPCObject* s_pLastTable;
        RPCObject* m_pNextTable;

        RPCObject(const std::type_info& type,
            std::initializer_list<rpc_method> table);
        RPCObject(rpc_id objectId);

        const std::string& GetObjectName() const;
        void Dispatch(const std::string& func, json::object& args);
    private:
        std::string m_ObjectName;
        std::vector<rpc_method> m_Methods;
        rpc_call m_Call;
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
        
        void Init();
    private:
        RPCType m_Type;
        tcp::endpoint m_Address;
        boost::thread m_RPCThread;
    };

    extern std::shared_ptr<RPC> rpc;
}

#endif