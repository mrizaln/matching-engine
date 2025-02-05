#pragma once

#include "price.hpp"
#include "util.hpp"

#include <fmt/format.h>
#include <fmt/std.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <vector>
#include <type_traits>

namespace match_engine
{
    using Clock     = std::chrono::system_clock;
    using Timestamp = Clock::time_point;

    // u64 to make the Order struct aligned with no padding
    enum class OrderType : al::u64
    {
        Buy,
        Sell,
    };

    constexpr std::string_view to_string(OrderType type)
    {
        switch (type) {
        case OrderType::Buy: return "Buy";
        case OrderType::Sell: return "Sell";
        default: [[unlikely]] return "???";
        }
    }

    constexpr std::optional<OrderType> order_type_from_str(std::string_view str)
    {
        if (str == "sell") {
            return OrderType::Sell;
        } else if (str == "buy") {
            return OrderType::Buy;
        }
        return std::nullopt;
    }

    class OrderId
    {
    public:
        using Inner = al::u64;

        static OrderId next() { return OrderId(++s_next); }    // starts from 1

        Inner inner() const { return m_inner; }

        std::strong_ordering operator<=>(const OrderId&) const = default;

    private:
        static inline std::atomic<Inner> s_next = 0;

        OrderId(Inner inner)
            : m_inner(inner)
        {
        }

        Inner m_inner;
    };

    struct Order
    {
        OrderId   m_id;
        OrderType m_type;
        Timestamp m_timestamp;
        Price     m_price;
        al::u64   m_quantity;
    };

    static_assert(std::has_unique_object_representations_v<Order>);
    static_assert(std::is_trivially_copyable_v<Order>);
    static_assert(std::is_trivially_move_constructible_v<Order>);
    static_assert(std::is_trivially_move_assignable_v<Order>);
    static_assert(std::is_trivially_destructible_v<Order>);

    class OrderBook
    {
    public:
        void add(Order order)
        {
            auto& orders = m_orders[order.m_price];
            orders.push_back(order);
        }

        /**
         * @brief Traverse all orders at a given price and apply a function to each order.
         *
         * @tparam Fn The function must return true to continue modifying the order or false to stop.
         * @param price The price to modify.
         * @param fn the function to apply to each order.
         * @return The number of orders modified.
         */
        template <typename Fn>
            requires std::invocable<Fn, Order&> and std::same_as<std::invoke_result_t<Fn, Order&>, bool>
        al::usize modify(Price price, Fn&& fn)
        {
            auto maybe_orders = m_orders.find(price);
            if (maybe_orders == m_orders.end()) {
                return 0;
            }

            auto to_be_deleted = std::vector<al::usize>{};

            auto& orders = maybe_orders->second;
            assert(!orders.empty());

            auto count = 0_usize;

            for (auto i : al::sv::iota(0_usize, orders.size())) {
                auto& order = orders[i];

                auto proceed = fn(order);
                ++count;

                if (order.m_quantity == 0) {
                    to_be_deleted.push_back(i);
                }
                if (not proceed) {
                    break;
                }
            }

            // delete the orders with quantity == 0 in single pass
            util::erase_by_indices(orders, to_be_deleted);

            if (orders.empty()) {
                m_orders.erase(maybe_orders);
            }

            return count;
        }

        const auto& inner() const { return m_orders; }

    private:
        std::unordered_map<Price, std::vector<Order>> m_orders;
    };
}

template <>
struct fmt::formatter<match_engine::Order> : fmt::formatter<std::string_view>
{
    auto format(const match_engine::Order& order, format_context& ctx) const
    {
        auto&& [id, type, timestamp, price, quantity] = order;
        return fmt::format_to(
            ctx.out(),
            "Order {{ id: {0}, type: {1}, timestamp: {2:%FT%TZ}, price: {3}, quantity: {4} }}",
            id.inner(),
            match_engine::to_string(type),
            timestamp,
            price.inner(),
            quantity
        );
    }
};
