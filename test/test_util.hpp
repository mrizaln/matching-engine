#pragma once

#include "match_engine.hpp"

#include <random>
#include <variant>
#include <algorithm>
#include <numeric>

namespace test_util
{
    namespace me = match_engine;
    namespace sr = me::al::sr;
    namespace sv = me::al::sv;

    template <typename... Fs>
    struct Overload : Fs...
    {
        using Fs::operator()...;
    };

    inline me::Order order_buy(me::Price price, me::al::u64 quantity)
    {
        return me::Order{ me::OrderId::next(), me::OrderType::Buy, me::Clock::now(), price, quantity };
    }

    inline me::Order order_sell(me::Price price, me::al::u64 quantity)
    {
        return me::Order{ me::OrderId::next(), me::OrderType::Sell, me::Clock::now(), price, quantity };
    }

    inline bool orderbook_empty(const me::OrderBook& orders)
    {
        return sr::all_of(orders.inner(), [](auto&& list) { return list.second.empty(); });
    }

    template <typename T, std::ranges::range R, typename Fn>
    T fold_left(R&& rng, T init, Fn&& fn)
    {
        return std::accumulate(std::ranges::begin(rng), std::ranges::end(rng), init, std::forward<Fn>(fn));
    }

    struct OrderGenType
    {
        // clang-format off
        struct Buy  {};
        struct Sell {};
        struct Both { double m_buy_ratio; };
        // clang-format on

        using Variant = std::variant<Buy, Sell, Both>;
    };

    class OrderGen
    {
    public:
        using Rng = std::mt19937;

        OrderGen(
            std::pair<me::Price, me::Price>     price_range,
            std::pair<me::al::u64, me::al::u64> quantity_range,
            OrderGenType::Variant               type
        )
            : m_price_dist{ price_range.first.inner(), price_range.second.inner() }
            , m_quantity_dist{ quantity_range.first, quantity_range.second }
            , m_type{ type }
        {
        }

        me::Order generate(Rng& rng)
        {
            auto price    = me::Price{ m_price_dist(rng) };
            auto quantity = m_quantity_dist(rng);
            auto type     = get_order_type(rng);

            return {
                .m_id        = me::OrderId::next(),
                .m_type      = type,
                .m_timestamp = me::Clock::now(),
                .m_price     = price,
                .m_quantity  = quantity,
            };
        }

    private:
        me::OrderType get_order_type(Rng& rng)
        {
            static thread_local auto dist = std::uniform_real_distribution<double>{ 0.0, 1.0 };

            auto visitor = Overload{
                [](OrderGenType::Buy) { return me::OrderType::Buy; },
                [](OrderGenType::Sell) { return me::OrderType::Sell; },
                [&](OrderGenType::Both both) {
                    return dist(rng) < both.m_buy_ratio ? me::OrderType::Buy : me::OrderType::Sell;
                },
            };

            return std::visit(visitor, m_type);
        }

        std::uniform_int_distribution<me::Price::Inner> m_price_dist;
        std::uniform_int_distribution<me::al::u64>      m_quantity_dist;
        OrderGenType::Variant                           m_type;
    };
}
