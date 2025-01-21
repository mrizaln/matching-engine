#pragma once

#include "aliases.hpp"
#include "async.hpp"

#include <functional>

namespace match_engine
{
    struct MessageProtocol
    {
        async::Awaitable<std::optional<std::string>> receive(async::TcpSocket& socket);
        async::Awaitable<bool>                       send(async::TcpSocket& socket, std::string_view message);
    };

    class Server
    {
    public:
        using OnReceiveFun = std::function<void(std::string)>;

        Server(async::IoContext& context, unsigned short port);
        ~Server();

        void run(OnReceiveFun&& on_receive);
        void stop();

    private:
        async::Awaitable<void> accept_connection();
        async::Awaitable<void> handle_connection(async::TcpSocket socket);

        al::u16           m_port;
        std::atomic<bool> m_running;

        async::IoContext&  m_context;
        async::TcpAcceptor m_acceptor;
        MessageProtocol    m_protocol;

        std::optional<OnReceiveFun> m_on_receive;
    };
}
