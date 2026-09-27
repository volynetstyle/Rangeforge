#pragma once

#include <array>
#include <bitset>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iosfwd>
#include <random>
#include <ratio>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace rangeforge {

/// Unsigned integer type suitable for object sizes, container lengths,
/// and memory offsets.
using usize = std::size_t;

/// Signed counterpart to `usize`, suitable for signed offsets and differences.
using isize = std::ptrdiff_t;

/// Fixed-width unsigned integer types.
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

/// Fixed-width signed integer types.
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

/// Standard floating-point types used by Rangeforge.
using f32 = float;
using f64 = double;

/// Owning UTF-agnostic byte string.
///
/// Encoding is intentionally not prescribed by the type.
using String = std::string;

/// Non-owning view over contiguous string data.
///
/// The referenced character sequence must outlive the view.
using StringView = std::string_view;

/// Platform-aware filesystem path.
using Path = std::filesystem::path;

/// Dynamically sized contiguous sequence.
template <class T> using Vector = std::vector<T>;

/// Fixed-size contiguous sequence containing exactly `Size` elements.
template <class T, usize Size> using Array = std::array<T, Size>;

/// Fixed-size compile-time bit container.
template <usize Size> using Bitset = std::bitset<Size>;

/// Monotonic clock used for elapsed-time and benchmark measurements.
///
/// `steady_clock` is intentionally used because benchmark timing must not be
/// affected by wall-clock adjustments.
using Clock = std::chrono::steady_clock;

/// Native duration representation of the Rangeforge monotonic clock.
using Duration = Clock::duration;

/// Millisecond duration.
using Milliseconds = std::chrono::milliseconds;

/// Nanosecond duration.
using Nanoseconds = std::chrono::nanoseconds;

/// Ratio used to express floating-point durations in milliseconds.
using MillisecondRatio = std::milli;

/// Generic duration type with explicitly controlled representation and period.
///
/// Useful when an algorithm or benchmark needs a unit not covered by the
/// predefined aliases above.
template <class Rep, class Period = std::ratio<1>>
using BasicDuration = std::chrono::duration<Rep, Period>;

/// Default deterministic pseudo-random number generator used by Rangeforge.
///
/// The generator is suitable for reproducible experiments when initialized
/// with a fixed seed.
using RandomEngine = std::mt19937_64;

/// Uniform integer distribution over an inclusive integer interval.
template <class Integer> using UniformIntegerDistribution = std::uniform_int_distribution<Integer>;

/// Base output stream type used by reporting and formatting facilities.
using OutputStream = std::ostream;

/// Output stream that writes to a file.
using OutputFileStream = std::ofstream;

/// Output stream that writes to a string buffer.
using OutputStringStream = std::ostringstream;

/// Base stream state type.
using IoState = std::ios;

/// Base type for standard exceptions.
using Exception = std::exception;

/// Error representing a failure detected during program execution.
using RuntimeError = std::runtime_error;

/// Error representing an invalid argument supplied by a caller.
using InvalidArgument = std::invalid_argument;

/// Evaluates to `true` when `T` is an integral type.
///
/// This wrapper keeps type-trait usage inside the Rangeforge vocabulary and
/// allows its semantics to evolve independently if required.
template <class T> struct IsIntegral : std::bool_constant<std::is_integral_v<T>> {};

/// Convenience variable template for `IsIntegral<T>::value`.
template <class T> inline constexpr bool IsIntegralV = IsIntegral<T>::value;

/// Result of a single validation or correctness test.
struct TestResult {
    /// Human-readable identifier of the test.
    String name;

    /// `true` when the tested condition completed successfully.
    bool passed;

    /// Optional human-readable diagnostic information.
    ///
    /// Typically empty for successful tests and populated on failure.
    String detail;
};

/// Aggregate timing result produced by a benchmark run.
struct BenchmarkResult {
    /// Human-readable benchmark identifier.
    String name;

    /// Number of measured benchmark iterations.
    usize iterations;

    /// Total measured execution time in milliseconds.
    double totalMilliseconds;

    /// Average execution time per iteration in nanoseconds.
    double nanosecondsPerIteration;
};

} // namespace rangeforge
