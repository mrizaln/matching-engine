#pragma once

#include "order.hpp"
#include "price.hpp"

namespace match_engine
{
    class MatchEngine
    {
    public:
        std::vector<Order> match(Order order)
        {
            switch (order.m_type) {
            case OrderType::Buy: return match_buy_order(order);
            case OrderType::Sell: return match_sell_order(order);
            default: [[unlikely]] std::terminate();
            }
        }

        const OrderBook& buy_orders() const { return m_buy_orders; }
        const OrderBook& sell_orders() const { return m_sell_orders; }

    private:
        std::vector<Order> match_buy_order(Order buy)
        {
            auto maybe_sell = m_sell_orders.find(buy.m_price);
            if (maybe_sell == nullptr) {
                m_buy_orders.add(buy);
                return {};
            }

            auto successful_sell = std::vector<Order>{};

            maybe_sell->modify([&](Order& sell) {
                if (buy.m_quantity == 0) {
                    return false;
                }

                // allow partial fill
                auto amount      = std::min(buy.m_quantity, sell.m_quantity);
                buy.m_quantity  -= amount;
                sell.m_quantity -= amount;

                successful_sell.emplace_back(sell.m_id, sell.m_type, sell.m_timestamp, sell.m_price, amount);

                return true;
            });

            if (buy.m_quantity > 0) {
                m_buy_orders.add(buy);
            }

            return successful_sell;
        }

        std::vector<Order> match_sell_order(Order sell)
        {
            auto maybe_buy = m_buy_orders.find(sell.m_price);
            if (maybe_buy == nullptr) {
                m_sell_orders.add(sell);
                return {};
            }

            auto successful_buy = std::vector<Order>{};

            maybe_buy->modify([&](Order& buy) {
                if (sell.m_quantity == 0) {
                    return false;
                }

                // allow partial fill
                auto amount      = std::min(sell.m_quantity, buy.m_quantity);
                sell.m_quantity -= amount;
                buy.m_quantity  -= amount;

                successful_buy.emplace_back(buy.m_id, buy.m_type, buy.m_timestamp, buy.m_price, amount);

                return true;
            });

            if (sell.m_quantity > 0) {
                m_sell_orders.add(sell);
            }

            return successful_buy;
        }

        OrderBook m_buy_orders;
        OrderBook m_sell_orders;
    };
}
