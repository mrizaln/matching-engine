#include "application.hpp"

#include <fmt/core.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <charconv>

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fmt::println(stderr, "Usage: {} <port>", argv[0]);
        return 1;
    } else if (argc > 2) {
        fmt::println(stderr, "Too many arguments");
        fmt::println(stderr, "Usage: {} <port>", argv[0]);
        return 1;
    }

    auto probably_port = std::string_view{ argv[1] };

    using match_engine::operator""_u16;
    auto port      = 0_u16;
    auto [ptr, ec] = std::from_chars(probably_port.data(), probably_port.end(), port);
    if (ec != std::errc{}) {
        fmt::println(stderr, "Invalid port '{}': {}", probably_port, std::make_error_code(ec).message());
        fmt::println(stderr, "Usage: {} <port>", argv[0]);
        return 1;
    } else if (ptr != probably_port.end()) {
        fmt::println(stderr, "Invalid port '{}': contains non-numeric characters", probably_port);
        fmt::println(stderr, "Usage: {} <port>", argv[0]);
        return 1;
    }

    spdlog::set_default_logger(spdlog::stderr_color_st("logger"));
    spdlog::set_level(spdlog::level::info);

    try {
        auto application = Application{ port };
        application.run();
    } catch (const std::exception& e) {
        spdlog::error("Exception occured: {}", e.what());
        return 1;
    }
}
