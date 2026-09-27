#pragma once

#include <array>
#include <bitset>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <random>
#include <ratio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace rangeforge {

using usize = std::size_t;
using isize = std::ptrdiff_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using f32 = float;
using f64 = double;

using String = std::string;
using StringView = std::string_view;
template <class T> using Vec = std::vector<T>;
using Path = std::filesystem::path;
using OStream = std::ostream;
using RuntimeError = std::runtime_error;
using InvalidArgument = std::invalid_argument;
template <class T> using UniformIntDistribution = std::uniform_int_distribution<T>;
using Mt19937_64 = std::mt19937_64;
using IoState = std::ios;

template <class Rep, class Period = std::ratio<1>>
using DurationOf = std::chrono::duration<Rep, Period>;
using Duration = std::chrono::steady_clock::duration;
using SteadyClock = std::chrono::steady_clock;
using Milliseconds = std::chrono::milliseconds;
using Nanoseconds = std::chrono::nanoseconds;

template <class T, usize N> using Array = std::array<T, N>;
template <usize N> using BitSet = std::bitset<N>;

template <class T> struct is_integral : std::bool_constant<std::is_integral_v<T>> {};

struct TestResult {
    String name;
    bool passed;
    String detail;
};

struct BenchmarkResult {
    String name;
    usize iterations;
    double total_ms;
    double ns_per_iteration;
};

} // namespace rangeforge
