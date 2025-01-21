#include "test_util.hpp"

#include "match_engine.hpp"

#include <boost/ut.hpp>
#include <fmt/base.h>
#include <fmt/format.h>

#include <random>

namespace ut = boost::ut;
namespace me = match_engine;
namespace sr = me::al::sr;
namespace sv = me::al::sv;

using test_util::order_buy, test_util::order_sell, test_util::orderbook_empty;
using test_util::OrderGen, test_util::OrderGenType;

namespace match_engine
{
    inline std::ostream& operator<<(std::ostream& os, const Price& price)
    {
        return os << price.inner();
    }

    inline std::ostream& operator<<(std::ostream& os, const OrderId& id)
    {
        return os << id.inner();
    }

    inline std::ostream& operator<<(std::ostream& os, const Order& order)
    {
        auto&& [id, type, timestamp, price, quantity] = order;
        return os << "Order{ id=" << id.inner()                           //
                  << ", type=" << to_string(type)                         //
                  << ", time=" << timestamp.time_since_epoch().count()    //
                  << ", price=" << price.inner()                          //
                  << ", quantity=" << quantity << " }";
    }
}

template <>
struct fmt::formatter<me::Order> : fmt::formatter<std::string_view>
{
    auto format(const me::Order& order, format_context& ctx) const
    {
        auto&& [id, type, timestamp, price, quantity] = order;
        return fmt::format_to(
            ctx.out(),
            "Order{{ id={:>4}, type={:<4}, time={:>20}, price={:>4}, quantity={:>4} }}",
            id.inner(),
            to_string(type),
            timestamp.time_since_epoch().count(),
            price.inner(),
            quantity
        );
    }
};

int main()
{
    using namespace ut::operators;
    using namespace ut::literals;
    using ut::expect, ut::that, ut::throws, ut::nothrow;

    using me::operator""_price;

    "order matching engine should be able to match buy and sell orders"_test = [] {
        const auto buys = std::vector{
            order_buy(10_price, 10),
            order_buy(12_price, 10),
            order_buy(42_price, 10),
        };

        const auto sells = std::vector{
            order_sell(10_price, 10),
            order_sell(12_price, 10),
            order_sell(42_price, 10),
        };

        auto match_engine = me::MatchEngine{};

        "should match orders buys first then sells"_test = [&] {
            for (auto buy : buys) {
                expect(match_engine.match(buy).empty());
            }
            for (auto sell : sells) {
                auto matched = match_engine.match(sell);
                expect(that % matched.size() == 1);
                expect(that % matched[0].m_quantity == sell.m_quantity);
                expect(that % matched[0].m_price == sell.m_price);
            }

            expect(orderbook_empty(match_engine.buy_orders()));
            expect(orderbook_empty(match_engine.sell_orders()));
        };

        "should match orders sells first then buys"_test = [&] {
            for (auto sell : sells) {
                expect(match_engine.match(sell).empty());
            }
            for (auto buy : buys) {
                auto matched = match_engine.match(buy);
                expect(that % matched.size() == 1);
                expect(that % matched[0].m_quantity == buy.m_quantity);
                expect(that % matched[0].m_price == buy.m_price);
            }

            expect(orderbook_empty(match_engine.buy_orders()));
            expect(orderbook_empty(match_engine.sell_orders()));
        };

        "should match orders sells and buys interleaved"_test = [&] {
            for (auto i : sv::iota(0u, buys.size())) {
                auto buy  = buys[i];
                auto sell = sells[i];

                expect(match_engine.match(buy).empty());

                auto matched = match_engine.match(sell);
                expect(that % matched.size() == 1);
                expect(that % matched[0].m_quantity == sell.m_quantity);
                expect(that % matched[0].m_price == sell.m_price);
            }

            for (auto i : sv::iota(0u, buys.size())) {
                auto buy  = buys[i];
                auto sell = sells[i];

                expect(match_engine.match(sell).empty());

                auto matched = match_engine.match(buy);
                expect(that % matched.size() == 1);
                expect(that % matched[0].m_quantity == buy.m_quantity);
                expect(that % matched[0].m_price == buy.m_price);
            }

            expect(orderbook_empty(match_engine.buy_orders()));
            expect(orderbook_empty(match_engine.sell_orders()));
        };
    };

    "order matching engine should be able to match multiple orders"_test = [] {
        const auto buys = std::vector{
            order_buy(10_price, 1),
            order_buy(10_price, 2),
            order_buy(10_price, 4),
        };

        const auto sells = std::vector{
            order_sell(10_price, 1),
            order_sell(10_price, 2),
            order_sell(10_price, 4),
        };

        "should match a buy order with multiple sell orders"_test = [&] {
            auto match_engine = me::MatchEngine{};
            auto buy          = order_buy(10_price, 10);

            for (auto sell : sells) {
                expect(match_engine.match(sell).empty());
            }

            auto matched = match_engine.match(buy);
            expect(that % matched.size() == sells.size());
            for (auto i : sv::iota(0u, sells.size())) {
                expect(that % matched[i].m_quantity == sells[i].m_quantity);
                expect(that % matched[i].m_price == sells[i].m_price);
            }

            expect(orderbook_empty(match_engine.sell_orders()));
            expect(not orderbook_empty(match_engine.buy_orders()));

            expect(that % match_engine.buy_orders().inner().at(buy.m_price).size() == 1);
            expect(that % match_engine.buy_orders().inner().at(buy.m_price).top().m_quantity == 3);
        };

        "should match a sell order with multiple buy orders"_test = [&] {
            auto match_engine = me::MatchEngine{};
            auto sell         = order_sell(10_price, 10);

            for (auto buy : buys) {
                expect(match_engine.match(buy).empty());
            }

            auto matched = match_engine.match(sell);
            expect(that % matched.size() == buys.size());
            for (auto i : sv::iota(0u, buys.size())) {
                expect(that % matched[i].m_quantity == buys[i].m_quantity);
                expect(that % matched[i].m_price == buys[i].m_price);
            }

            expect(orderbook_empty(match_engine.buy_orders()));
            expect(not orderbook_empty(match_engine.sell_orders()));

            expect(that % match_engine.sell_orders().inner().at(sell.m_price).size() == 1);
            expect(that % match_engine.sell_orders().inner().at(sell.m_price).top().m_quantity == 3);
        };
    };

    "if there are no matching orders, the order should remain in the order book"_test = [] {
        auto orders = std::vector{
            order_buy(1_price, 10),   order_buy(2_price, 10),   order_buy(3_price, 10),
            order_buy(4_price, 10),   order_buy(5_price, 10),   order_sell(6_price, 10),
            order_buy(7_price, 10),   order_sell(8_price, 10),  order_buy(9_price, 10),
            order_sell(10_price, 10), order_sell(11_price, 10), order_sell(12_price, 10),
            order_buy(13_price, 10),  order_buy(14_price, 10),  order_buy(15_price, 10),
            order_buy(16_price, 10),  order_sell(17_price, 10), order_sell(18_price, 10),
            order_sell(19_price, 10), order_sell(20_price, 10), order_sell(21_price, 10),
            order_sell(22_price, 10), order_sell(23_price, 10), order_buy(24_price, 10),
            order_buy(25_price, 10),  order_sell(26_price, 10), order_sell(27_price, 10),
            order_sell(28_price, 10), order_sell(29_price, 10), order_sell(30_price, 10),
            order_sell(31_price, 10), order_buy(32_price, 10),  order_buy(33_price, 10),
        };

        auto gen = std::mt19937{ std::random_device{}() };

        // shuffling the orders to test the order book
        for (auto _ : sv::iota(0u, 10u)) {
            sr::shuffle(orders, gen);

            auto match_engine = me::MatchEngine{};

            for (auto order : orders) {
                expect(match_engine.match(order).empty());
            }

            expect(not orderbook_empty(match_engine.buy_orders()));
            expect(not orderbook_empty(match_engine.sell_orders()));
        }
    };

    "if an order match multiple orders, the order should be performed to the older orders first"_test = [] {
        const auto sells = std::vector{
            order_sell(10_price, 10),
            order_sell(10_price, 10),
            order_sell(10_price, 10),
        };

        const auto buys = std::vector{
            order_buy(10_price, 10),
            order_buy(10_price, 10),
            order_buy(10_price, 10),
        };

        "a buy order that match multiple sell orders but can only fullfill one sell order"_test = [&] {
            auto       match_engine = me::MatchEngine{};
            const auto buy          = order_buy(10_price, 10);

            for (auto sell : sells) {
                expect(match_engine.match(sell).empty());
            }

            auto matched = match_engine.match(buy);
            expect(that % matched.size() == 1);
            expect(that % matched[0].m_quantity == 10);
            expect(that % matched[0].m_price == 10_price);
            expect(that % matched[0].m_id == sells[0].m_id);
            expect(that % matched[0].m_timestamp == sells[0].m_timestamp);

            expect(sr::all_of(sells | sv::drop(1), [&](auto&& sell) {
                return matched[0].m_timestamp < sell.m_timestamp;
            }));
        };

        "a sell order that match multiple buy orders but can only fullfill one buy order"_test = [&] {
            auto       match_engine = me::MatchEngine{};
            const auto sell         = order_sell(10_price, 10);

            for (auto buy : buys) {
                expect(match_engine.match(buy).empty());
            }

            auto matched = match_engine.match(sell);
            expect(that % matched.size() == 1);
            expect(that % matched[0].m_quantity == 10);
            expect(that % matched[0].m_price == 10_price);
            expect(that % matched[0].m_id == buys[0].m_id);
            expect(that % matched[0].m_timestamp == buys[0].m_timestamp);

            expect(sr::all_of(buys | sv::drop(1), [&](auto&& buy) {
                return matched[0].m_timestamp < buy.m_timestamp;
            }));
        };

        "a buy order that match multiple sell orders and can fulfill multiple orders"_test = [&] {
            auto       match_engine = me::MatchEngine{};
            const auto buy          = order_buy(10_price, 15);    // can fill two sell orders: one partially

            for (auto sell : sells) {
                expect(match_engine.match(sell).empty());
            }

            auto matched = match_engine.match(buy);
            expect(that % matched.size() == 2);
            expect(that % matched[0].m_price == 10_price);
            expect(that % matched[1].m_price == 10_price);
            expect(that % matched[0].m_quantity == 10);
            expect(that % matched[1].m_quantity == 5);
            expect(that % matched[0].m_id == sells[0].m_id);
            expect(that % matched[1].m_id == sells[1].m_id);
            expect(that % matched[0].m_timestamp == sells[0].m_timestamp);
            expect(that % matched[1].m_timestamp == sells[1].m_timestamp);

            expect(matched[0].m_timestamp < matched[1].m_timestamp);
            expect(matched[1].m_timestamp < sells[2].m_timestamp);

            expect(match_engine.sell_orders().inner().at(10_price).top().m_quantity == 5);
        };

        "a sell order that match multiple buy orders and can fulfill multiple orders"_test = [&] {
            auto       match_engine = me::MatchEngine{};
            const auto sell         = order_sell(10_price, 15);    // can fill two buy orders: one partially

            for (auto buy : buys) {
                expect(match_engine.match(buy).empty());
            }

            auto matched = match_engine.match(sell);
            expect(that % matched.size() == 2);
            expect(that % matched[0].m_price == 10_price);
            expect(that % matched[1].m_price == 10_price);
            expect(that % matched[0].m_quantity == 10);
            expect(that % matched[1].m_quantity == 5);
            expect(that % matched[0].m_id == buys[0].m_id);
            expect(that % matched[1].m_id == buys[1].m_id);
            expect(that % matched[0].m_timestamp == buys[0].m_timestamp);
            expect(that % matched[1].m_timestamp == buys[1].m_timestamp);

            expect(matched[0].m_timestamp < matched[1].m_timestamp);
            expect(matched[1].m_timestamp < buys[2].m_timestamp);

            expect(match_engine.buy_orders().inner().at(10_price).top().m_quantity == 5);
        };
    };

    // NOTE: remove the ut::skip to run the test/benchmark
    ut::skip / "order matching engine should be able to handle high volume trades at high speed"_test = [&] {
        auto match_engine = me::MatchEngine{};
        auto order_gen    = OrderGen{ { 1_price, 10000_price }, { 1, 1000 }, OrderGenType::Both{ 0.5 } };

        auto rng         = std::mt19937{ std::random_device{}() };
        auto order_count = 0ul;
        auto match_count = 0ul;

        auto start = me::Clock::now();

        for (auto _ : sv::iota(0u, 10'000'000u)) {
            auto order   = order_gen.generate(rng);
            auto matched = match_engine.match(order);

            order_count += 1;
            match_count += matched.size() > 0;

            // if (not matched.empty()) {
            //     fmt::println("Matched order:");
            //     fmt::println("\t{:<4}: {}", to_string(order.m_type), order);
            //     for (auto&& match : matched) {
            //         fmt::println("\t{:<4}: {}", to_string(match.m_type), match);
            //     }
            // }
        }

        auto elapsed = me::Clock::now() - start;
        auto sec     = std::chrono::duration_cast<std::chrono::duration<double>>(elapsed);

        // clang-format off
        fmt::println("Order statistics:");
        fmt::println("\tTotal orders    : {}", order_count);
        fmt::println("\tMatched orders  : {}", match_count);
        fmt::println("\tMatch rate      : {:.2f}%", (static_cast<double>(match_count) / static_cast<double>(order_count)) * 100);
        fmt::println("\tUnmatched orders: {}", order_count - match_count);
        fmt::println("\tElapsed time    : {:.2f} seconds", sec.count());
        // clang-format on
    };
}
