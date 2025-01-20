#pragma once

#include "aliases.hpp"

#include <compare>
#include <functional>

namespace match_engine
{
    class Price
    {
    public:
        // change the inner value with fixed point if needed, for now I'll just use integers
        using Inner = al::u64;

        constexpr explicit Price(Inner inner)
            : m_inner(inner)
        {
        }

        const Inner& inner() const { return m_inner; }

        std::strong_ordering operator<=>(const Price&) const = default;

        Inner operator+=(const Price& other) { return m_inner += other.m_inner; }
        Inner operator-=(const Price& other) { return m_inner -= other.m_inner; }
        Inner operator*=(const Price& other) { return m_inner *= other.m_inner; }
        Inner operator/=(const Price& other) { return m_inner /= other.m_inner; }

        Inner operator+(const Price& other) const { return m_inner + other.m_inner; }
        Inner operator-(const Price& other) const { return m_inner - other.m_inner; }
        Inner operator*(const Price& other) const { return m_inner * other.m_inner; }
        Inner operator/(const Price& other) const { return m_inner / other.m_inner; }

        Inner operator-() const { return -m_inner; }

    private:
        Inner m_inner;
    };

    constexpr Price operator""_price(unsigned long long value)
    {
        return Price(static_cast<Price::Inner>(value));
    }
}

template <>
struct std::hash<match_engine::Price>
{
    std::size_t operator()(const match_engine::Price& price) const noexcept
    {
        auto hash = std::hash<match_engine::Price::Inner>();
        return hash(price.inner());
    };
};
