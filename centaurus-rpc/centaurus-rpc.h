#ifndef __CENTAURUS_RPC_H
#define __CENTAURUS_RPC_H

#include <string>
#include <memory>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace centaurus
{
    namespace rpc
    {
        //RPC API Table

        using tcp = boost::asio::ip::tcp;
        namespace beast = boost::beast;
        namespace http = boost::beast::http;

        class Session : public std::enable_shared_from_this<Session>
        {
        public:
            enum SessionType {
                RPC_SERVER,
                RPC_CLIENT
            };
            
            Session(tcp::socket socket);
        private:
            tcp::socket m_Socket;
            beast::flat_buffer m_Buffer{ 4096 };
            http::request<http::dynamic_body> m_Request;
            http::response<http::dynamic_body> m_Response;
        };

        void test_server();
        void test_client();
    }
}

#endif