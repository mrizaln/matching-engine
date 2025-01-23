#pragma once

#include "match_engine.hpp"
#include "server.hpp"

#include <rapidjson/document.h>
#include <spdlog/spdlog.h>

#include <vector>

class Application
{
public:
    Application(match_engine::al::u16 port)
        : m_server{ m_context, port }
        , m_engine{}
    {
    }

    void run()
    {
        m_server.run([&](std::string string) {
            auto maybe_json = parse_json(string);
            if (not maybe_json.has_value()) {
                return;
            }

            auto& parsed = maybe_json.value();

            auto type = match_engine::order_type_from_str(parsed["type"].GetString());
            if (not type.has_value()) {
                spdlog::warn(
                    "Received an unknown order type: '{}' (expected 'buy' or 'sell'), ignoring...",
                    parsed["type"].GetString()
                );
                return;
            }
            auto price    = parsed["price"].GetUint64();
            auto quantity = parsed["quantity"].GetUint64();

            auto order = match_engine::Order{
                .m_id        = match_engine::OrderId::next(),
                .m_type      = type.value(),
                .m_timestamp = match_engine::Clock::now(),
                .m_price     = match_engine::Price{ price },
                .m_quantity  = quantity,
            };

            // spdlog::info("Incoming order: {}", order);

            auto matched = m_engine.match(order);
            if (not matched.empty()) {
                fmt::println("Matched orders:");
                fmt::println("\t{}", order);
                for (const auto& order : matched) {
                    fmt::println("\t{}", order);
                }
            }
        });

        m_context.run();
    }

private:
    std::optional<rapidjson::Document> parse_json(std::string_view string) const
    {
        auto parsed = rapidjson::Document{};
        parsed.Parse(string.data(), string.size());

        if (not parsed.HasParseError() and parsed.IsObject() and parsed.HasMember("price")
            and parsed["price"].IsUint64() and parsed.HasMember("quantity") and parsed["quantity"].IsUint64()
            and parsed.HasMember("type") and parsed["type"].IsString()) {
            return parsed;
        }

        spdlog::warn("Received a string that is not in a correctly formatted json, ignoring...");
        return std::nullopt;
    }

    match_engine::async::IoContext m_context;
    match_engine::Server           m_server;
    match_engine::MatchEngine      m_engine;
};
