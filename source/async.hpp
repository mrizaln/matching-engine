#pragma once

#include <asio.hpp>
#include <asio/experimental/awaitable_operators.hpp>

namespace match_engine::async
{
    using DefaultToken = asio::as_tuple_t<asio::use_awaitable_t<>>;
    using TcpAcceptor  = DefaultToken::as_default_on_t<asio::ip::tcp::acceptor>;
    using TcpSocket    = DefaultToken::as_default_on_t<asio::ip::tcp::socket>;
    using SteadyTimer  = DefaultToken::as_default_on_t<asio::steady_timer>;
    using IoContext    = asio::io_context;
    using Executor     = IoContext::executor_type;
    using Error        = asio::error_code;

    template <typename T>
    using Awaitable = asio::awaitable<T>;

    using asio::ip::tcp;

    using asio::buffer;
    using asio::post;

    template <typename Exec, typename Awaited>
    auto spawn(Exec&& ex, Awaited&& func)
    {
        return asio::co_spawn(std::forward<Exec>(ex), std::forward<Awaited>(func), asio::detached);
    }

    template <typename Exec, typename Awaited, typename Completion>
    auto spawn(Exec&& ex, Awaited&& func, Completion&& completion)
    {
        return asio::co_spawn(
            std::forward<Exec>(ex),    //
            std::forward<Awaited>(func),
            std::forward<Completion>(completion)
        );
    }

    namespace error     = asio::error;
    namespace await_ops = asio::experimental::awaitable_operators;
    namespace this_coro = asio::this_coro;

    namespace detail
    {
        inline const auto htonl = asio::detail::socket_ops::host_to_network_long;
        inline const auto ntohl = asio::detail::socket_ops::network_to_host_long;
    }
}
