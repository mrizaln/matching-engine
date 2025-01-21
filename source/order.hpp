#pragma once

#include "price.hpp"

#include <atomic>
#include <chrono>
#include <queue>
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
        }
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

    struct OldOrderFirstComp
    {
        bool operator()(const Order& lhs, const Order& rhs) const noexcept
        {
            return lhs.m_timestamp > rhs.m_timestamp;
        }
    };

    // NOTE: I need a priority queue that allows me to remove an element on the middle. inspired by
    // https://stackoverflow.com/a/36711682/16506263
    class OrderList : public std::priority_queue<Order, std::vector<Order>, OldOrderFirstComp>
    {
    public:
        // Fn should return true to continue modifying, false to stop immediately
        template <typename Fn>
            requires std::invocable<Fn, Order&> and std::same_as<std::invoke_result_t<Fn, Order&>, bool>
        void modify(Fn&& fn)
        {
            auto& container = this->c;
            // auto& comp      = this->comp;

            for (auto& order : container) {
                if (not fn(order)) {
                    break;
                }
            }

            std::erase_if(container, [](auto&& order) { return order.m_quantity == 0; });
            // al::sr::make_heap(container, comp);    // this step might not be necessary
        }
    };

    class OrderBook
    {
    public:
        void add(Order order)
        {
            auto& orders = m_orders[order.m_price];
            orders.push(order);
        }

        OrderList* find(Price price)
        {
            auto orders = m_orders.find(price);
            return orders != m_orders.end() ? &orders->second : nullptr;
        }

        const auto& inner() const { return m_orders; }

    private:
        std::unordered_map<Price, OrderList> m_orders;
    };
}
