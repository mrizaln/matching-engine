#include "server.hpp"

#include <fmt/format.h>
#include <spdlog/spdlog.h>

using NetLongCharArr = std::array<char, 4>;

namespace
{
    NetLongCharArr to_net_long(std::size_t len)
    {
        const auto htonl    = match_engine::async::detail::htonl;
        const auto net_long = htonl(static_cast<unsigned int>(len));
        return std::bit_cast<NetLongCharArr>(net_long);
    }

    std::size_t from_net_long(const NetLongCharArr& arr)
    {
        const auto ntohl = match_engine::async::detail::ntohl;
        return ntohl(std::bit_cast<unsigned int>(arr));
    }
}

namespace match_engine
{
    async::Awaitable<std::optional<std::string>> MessageProtocol::receive(async::TcpSocket& socket)
    {
        auto len_arr      = NetLongCharArr{};
        auto [err, bytes] = co_await socket.async_receive(async::buffer(len_arr));    // handled later
        if (bytes == 0) {
            spdlog::warn("(MessageProtocol) No bytes sent! Possible connection loss!");
            co_return std::nullopt;
        } else if (err) {
            spdlog::error("(MessageProtocol) Send failure: {}", err.message());
            co_return std::nullopt;
        }

        auto message_len = from_net_long(len_arr);
        auto message     = std::string(message_len, '\0');

        // TODO: make sure to handle the case where the message might be not fully received
        std::tie(err, bytes) = co_await socket.async_receive(async::buffer(message));
        if (bytes == 0) {
            spdlog::warn("(MessageProtocol) No bytes received! Possible connection loss!");
            co_return std::nullopt;
        } else if (bytes < message_len) {
            spdlog::warn("(MessageProtocol) Incomplete message received, ignoring...!");
            co_return std::nullopt;
        } else if (err) {
            spdlog::error("(MessageProtocol) Receive failure: {}", err.message());
            co_return std::nullopt;
        }

        co_return message;
    }

    async::Awaitable<bool> MessageProtocol::send(async::TcpSocket& socket, std::string_view message)
    {
        auto message_len  = to_net_long(message.length());
        auto [err, bytes] = co_await socket.async_send(async::buffer(message_len));
        if (bytes == 0) {
            spdlog::warn("(MessageProtocol) No bytes sent! Possible connection loss!");
            co_return false;
        } else if (err) {
            spdlog::error("(MessageProtocol) Send failure: {}", err.message());
            co_return false;
        }

        auto sent_bytes = 0ul;
        while (sent_bytes < message.size()) {
            auto buffer       = async::buffer(message.data() + sent_bytes, message.size() - sent_bytes);
            auto [err, bytes] = co_await socket.async_send(buffer);
            if (bytes == 0) {
                spdlog::warn("(MessageProtocol) No bytes sent! Possible connection loss!");
                co_return false;
            } else if (err) {
                spdlog::error("(MessageProtocol) Send failure: {}", err.message());
                co_return false;
            }
            sent_bytes += bytes;
        }

        co_return true;
    }
}

namespace match_engine
{
    Server::Server(async::IoContext& context, al::u16 port)
        : m_port{ port }
        , m_running{ false }
        , m_context{ context }
        , m_acceptor{ context, { async::tcp::v4(), port } }
    {
    }

    Server::~Server()
    {
        if (m_running) {
            stop();
        }
    }

    void Server::run(OnReceiveFun&& on_receive)
    {
        m_on_receive = std::move(on_receive);

        spdlog::info("(TcpServer) Starting server at port {} ...", m_port);
        const auto completion = [](std::exception_ptr exception) {
            if (exception) {
                std::rethrow_exception(exception);
            }
        };

        async::spawn(m_context, accept_connection(), completion);
    }

    void Server::stop()
    {
        m_running = false;
        spdlog::info("(TcpServer) Stopping server at port {} ...", m_port);

        async::post(m_context, [this] {
            m_acceptor.cancel();
            m_acceptor.close();
        });
    }

    async::Awaitable<void> Server::accept_connection()
    {
        m_running = true;
        spdlog::info("(TcpServer) Listening for incoming connections ...");

        while (m_running) {
            auto [err, remote] = co_await m_acceptor.async_accept();
            if (err) {
                if (err == async::error::operation_aborted) {
                    spdlog::info("(TcpServer) Acceptor cancelled: {}", err.message());
                    break;
                }
                spdlog::error("(TcpServer) Accept failure: {}", err.message());
                throw std::runtime_error{ fmt::format("(TcpServer) Accept failure: {}", err.message()) };
            }

            co_await handle_connection(std::move(remote));
        }
    }

    async::Awaitable<void> Server::handle_connection(async::TcpSocket socket)
    {
        auto message = co_await m_protocol.receive(socket);
        if (not message.has_value()) {
            spdlog::warn("(TcpServer) No message received! Possible connection loss!");
            co_return;
        }

        assert(m_on_receive.has_value() && "OnReceive function must be set!");
        (*m_on_receive)(std::move(message).value());
    }
}
