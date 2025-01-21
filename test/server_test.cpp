#include "server.hpp"

#include <fmt/core.h>
#include <boost/ut.hpp>

#include <span>
#include <thread>
#include <vector>

namespace ut = boost::ut;
namespace me = match_engine;
namespace sv = me::al::sv;

void client_fun(std::span<std::string_view> input)
{
    auto context  = me::async::IoContext{};
    auto resolver = me::async::tcp::resolver{ context };

    auto endpoints = resolver.resolve("localhost", "8080");

    me::async::spawn(context, [&]() -> me::async::Awaitable<void> {
        auto protocol = me::MessageProtocol{};

        for (auto str : input) {
            auto socket = me::async::TcpSocket{ context };
            asio::connect(socket, endpoints);

            co_await protocol.send(socket, str);

            using std::chrono_literals::operator""ms;
            std::this_thread::sleep_for(10ms);
        }
    });

    context.run();
}

int main()
{
    auto server_context = me::async::IoContext{};
    auto server         = me::Server{ server_context, 8080 };
    auto server_output  = std::vector<std::string>{};

    server.run([&](std::string string) { server_output.push_back(std::move(string)); });
    auto server_thread = std::jthread{ [&] { server_context.run(); } };

    auto client_input = std::array<std::string_view, 7>{
        "Hello, world!",
        "This is a test.",
        "Goodbye, world!",
        "The quick brown fox jumps over the lazy dog.",
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit.",

        R"json( { "price": 1210, "quantity": 330 } )json",

        "A black hole is a region of spacetime wherein gravity is so strong that no matter or "
        "electromagnetic energy (e.g. light) can escape it.[2] Albert Einstein's theory of general "
        "relativity predicts that a sufficiently compact mass can deform spacetime to form a black "
        "hole.[3][4] The boundary of no escape is called the event horizon. A black hole has a great effect "
        "on the fate and circumstances of an object crossing it, but it has no locally detectable features "
        "according to general relativity.[5] In many ways, a black hole acts like an ideal black body, as it "
        "reflects no light.[6][7] Quantum field theory in curved spacetime predicts that event horizons emit "
        "Hawking radiation, with the same spectrum as a black body of a temperature inversely proportional "
        "to its mass. This temperature is of the order of billionths of a kelvin for stellar black holes, "
        "making it essentially impossible to observe directly.",
    };

    auto client_thread = std::jthread{ client_fun, std::span<std::string_view>{ client_input } };

    client_thread.join();

    server.stop();
    server_thread.join();

    using namespace ut::operators;
    using namespace ut::literals;
    using ut::expect, ut::that, ut::throws, ut::nothrow;

    "server should receive all messages from the client"_test = [&] {
        expect(that % server_output.size() == client_input.size());
        for (auto i : sv::iota(0u, client_input.size())) {
            expect(that % server_output[i] == client_input[i]);
        }
    };
}
