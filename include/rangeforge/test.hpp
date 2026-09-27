#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <rangeforge/types.hpp>

namespace rangeforge {

class TestRunner {
  public:
    template <class F> void test(String name, F &&body) {
        try {
            std::invoke(std::forward<F>(body));
            results_.push_back({std::move(name), true, {}});
        } catch (const std::exception &e) {
            results_.push_back({std::move(name), false, e.what()});
        } catch (...) {
            results_.push_back({std::move(name), false, "unknown exception"});
        }
    }

    [[nodiscard]] i32 report(OStream &out = std::cout) const {
        usize passed = 0;
        for (const auto &r : results_) {
            out << (r.passed ? "[PASS] " : "[FAIL] ") << r.name;
            if (!r.detail.empty())
                out << ": " << r.detail;
            out << '\n';
            passed += r.passed;
        }
        out << passed << '/' << results_.size() << " tests passed\n";
        return passed == results_.size() ? 0 : 1;
    }

  private:
    Vec<TestResult> results_;
};

template <class A, class B>
void require_equal(const A &actual, const B &expected, StringView expression = "values differ") {
    if (!(actual == expected)) {
        std::ostringstream msg;
        msg << expression;
        throw RuntimeError(msg.str());
    }
}

inline void require(bool condition, StringView message = "requirement failed") {
    if (!condition)
        throw RuntimeError(String(message));
}

class Random {
  public:
    explicit Random(u64 seed = 0x52414E4745464F52ULL) : seed_(seed), engine_(seed) {}
    [[nodiscard]] u64 seed() const noexcept { return seed_; }

    template <class Int> Int integer(Int low, Int high) {
        static_assert(is_integral<Int>::value, "Random::integer requires an integral type");
        return UniformIntDistribution<Int>(low, high)(engine_);
    }

    template <class Container, class Int> Container integers(usize count, Int low, Int high) {
        Container result;
        if constexpr (requires { result.reserve(count); })
            result.reserve(count);
        for (usize i = 0; i < count; ++i)
            result.push_back(integer<Int>(low, high));
        return result;
    }

  private:
    u64 seed_;
    Mt19937_64 engine_;
};

template <class Input, class Oracle, class Candidate, class Format>
bool differential(usize cases, Random &random, Oracle &&oracle, Candidate &&candidate,
                  Format &&format, const Path &failure_file = "counterexample.txt") {
    for (usize i = 0; i < cases; ++i) {

        Input input = std::invoke(format, random, i);
        const auto expected = std::invoke(oracle, input);
        const auto actual = std::invoke(candidate, input);

        if (!(actual == expected)) {
            std::ofstream out(failure_file, std::ios::binary);
            if (!out)
                throw RuntimeError("cannot write counterexample: " + failure_file.string());
            out << "case " << i << "\nseed " << random.seed() << "\n";
            if constexpr (requires { out << input; }) {
                out << input;
            } else if constexpr (requires {
                                     input.begin();
                                     input.end();
                                 }) {
                for (const auto &value : input)
                    out << value << ' ';
                out << '\n';
            } else {
                out << "<input has no stream formatter>\n";
            }
            return false;
        }
    }
    return true;
}

template <class F> BenchmarkResult benchmark(String name, usize iterations, F &&function) {
    if (iterations == 0)
        throw InvalidArgument("benchmark iterations must be positive");
    const auto start = std::chrono::steady_clock::now();
    for (usize i = 0; i < iterations; ++i)
        std::invoke(function, i);
    const auto elapsed =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    return {std::move(name), iterations, elapsed, elapsed * 1'000'000.0 / iterations};
}

inline void print_benchmark(const BenchmarkResult &result, OStream &out = std::cout) {
    out << result.name << ": " << result.iterations << " iterations, " << std::fixed
        << std::setprecision(3) << result.total_ms << " ms total, " << result.ns_per_iteration
        << " ns/iteration\n";
}

} // namespace rangeforge
