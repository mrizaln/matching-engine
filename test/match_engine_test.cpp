#include "match_engine.hpp"

#include <boost/ut.hpp>

#include <random>

namespace ut = boost::ut;
namespace me = match_engine;
namespace sr = me::al::sr;
namespace sv = me::al::sv;

namespace match_engine
{
    std::ostream& operator<<(std::ostream& os, const me::Price& price)
    {
        return os << price.inner();
    }
}

me::Order order_buy(me::Price price, me::al::u64 quantity)
{
    return me::Order{ me::OrderId::next(), me::OrderType::Buy, me::Clock::now(), price, quantity };
}

me::Order order_sell(me::Price price, me::al::u64 quantity)
{
    return me::Order{ me::OrderId::next(), me::OrderType::Sell, me::Clock::now(), price, quantity };
}

bool orders_empty(const me::OrderBook& orders)
{
    return sr::all_of(orders.inner() | sv::values, [](auto&& list) { return list.empty(); });
}

int main()
{
    using namespace ut::operators;
    using namespace ut::literals;
    using ut::expect, ut::that, ut::throws, ut::nothrow;

    using me::operator""_price;

    "order match engine should be able to match buy and sell orders"_test = [] {
        const auto buys = std::vector<me::Order>{
            order_buy(10_price, 10),
            order_buy(12_price, 10),
            order_buy(42_price, 10),
        };

        const auto sells = std::vector<me::Order>{
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

            expect(orders_empty(match_engine.buy_orders()));
            expect(orders_empty(match_engine.sell_orders()));
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

            expect(orders_empty(match_engine.buy_orders()));
            expect(orders_empty(match_engine.sell_orders()));
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

            expect(orders_empty(match_engine.buy_orders()));
            expect(orders_empty(match_engine.sell_orders()));
        };
    };

    "order matching engine should be able to match multiple orders"_test = [] {
        const auto buys = std::vector<me::Order>{
            order_buy(10_price, 1),
            order_buy(10_price, 2),
            order_buy(10_price, 4),
        };

        const auto sells = std::vector<me::Order>{
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

            expect(orders_empty(match_engine.sell_orders()));
            expect(not orders_empty(match_engine.buy_orders()));

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

            expect(orders_empty(match_engine.buy_orders()));
            expect(not orders_empty(match_engine.sell_orders()));

            expect(that % match_engine.sell_orders().inner().at(sell.m_price).size() == 1);
            expect(that % match_engine.sell_orders().inner().at(sell.m_price).top().m_quantity == 3);
        };
    };

    "if there are no matching orders, the order should be added to the order book"_test = [] {
        auto orders = std::vector<me::Order>{
            order_buy(10_price, 34),  order_buy(12_price, 10),  order_buy(42_price, 92),
            order_sell(11_price, 23), order_sell(13_price, 78), order_sell(43_price, 56),
        };

        auto gen = std::mt19937{ std::random_device{}() };

        // shuffling the orders to test the order book
        for (auto _ : sv::iota(0u, 10u)) {
            sr::shuffle(orders, gen);

            auto match_engine = me::MatchEngine{};

            for (auto order : orders) {
                expect(match_engine.match(order).empty());
            }

            expect(not orders_empty(match_engine.buy_orders()));
            expect(not orders_empty(match_engine.sell_orders()));
        }
    };
}
