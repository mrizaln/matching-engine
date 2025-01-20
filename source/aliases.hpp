#pragma once

#include <cstdint>
#include <ranges>

namespace match_engine::al
{
    using i8  = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;

    using u8  = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    using usize = std::size_t;
    using isize = std::ptrdiff_t;

    using f32 = float;
    using f64 = double;

    namespace sr = std::ranges;
    namespace sv = std::views;
}

namespace match_engine::inline lit
{
    // clang-format off
    constexpr al::i8  operator""_i8 (unsigned long long value) { return static_cast<al::i8 >(value); }
    constexpr al::i16 operator""_i16(unsigned long long value) { return static_cast<al::i16>(value); }
    constexpr al::i32 operator""_i32(unsigned long long value) { return static_cast<al::i32>(value); }
    constexpr al::i64 operator""_i64(unsigned long long value) { return static_cast<al::i64>(value); }

    constexpr al::u8  operator""_u8 (unsigned long long value) { return static_cast<al::u8 >(value); }
    constexpr al::u16 operator""_u16(unsigned long long value) { return static_cast<al::u16>(value); }
    constexpr al::u32 operator""_u32(unsigned long long value) { return static_cast<al::u32>(value); }
    constexpr al::u64 operator""_u64(unsigned long long value) { return static_cast<al::u64>(value); }

    constexpr al::usize operator""_usize(unsigned long long value) { return static_cast<al::usize>(value); }
    constexpr al::isize operator""_isize(unsigned long long value) { return static_cast<al::isize>(value); }

    constexpr al::f32 operator""_f32(long double value) { return static_cast<al::f32>(value); }
    constexpr al::f64 operator""_f64(long double value) { return static_cast<al::f64>(value); }
    // clang-format on
}
