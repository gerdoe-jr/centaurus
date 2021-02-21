#ifndef __CENTAURUS_RPC_H
#define __CENTAURUS_RPC_H

#include <stdint.h>
#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <typeinfo>
#include <exception>
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

    struct rpc_call {
        rpc_id m_MethodId;
        std::vector<rpc_value> m_Args;
        rpc_value m_Return;
    };

    class RPCTable;

    template<typename T>
    class RPCBase
    {
    public:
        struct rpc_method {
            std::string m_MethodName;
            rpc_value_type m_ReturnType;
            std::vector<rpc_param> m_Params;
            std::function<void(T*)> m_Function;

            inline void operator()(T* pC) { m_Function(pC); }
        };

        virtual RPCTable* Table() = 0;

        void Dispatch(rpc_call& call, json::value& args);
    };

    class RPCTable : public RPCBase<RPCTable>
    {
    public:
        static RPCTable* s_pFirstTable;
        static RPCTable* s_pLastTable;
        RPCTable* m_pNextTable;

        //RPCTable(const std::type_info& type,
        //    std::initializer_list<rpc_method> table);
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
        rpc_id m_TableId;
        std::string m_TableName;
        std::vector<rpc_method> m_Methods;
    };

    /*template<typename T>
    class RPCObject
    {
    public:
        using rpc_method = rpc_base_method<T>;

        RPCObject();
        RPCObject(rpc_id objectId);
    };*/

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
        RPCTable* Table(rpc_id tableId);
        RPCTable* Table(const std::string& name);

        void Dispatch(rpc_call& call, rpc_id tableId, rpc_id methodId,
            json::object& args);
    private:
        RPCType m_Type;
        tcp::endpoint m_Address;
        boost::thread m_RPCThread;
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
}

#endif