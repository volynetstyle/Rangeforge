#pragma once

#include <functional>
#include <iomanip>
#include <iostream>
#include <utility>

#include <rangeforge/types.hpp>

namespace rangeforge {

using std::cout;
using std::fixed;
using std::forward;
using std::invoke;
using std::move;
using std::setprecision;

class TestRunner {
  public:
    template <class F> void test(String name, F &&body) {
        try {
            invoke(forward<F>(body));
            results_.push_back({move(name), true, {}});
        } catch (const Exception &e) {
            results_.push_back({move(name), false, e.what()});
        } catch (...) {
            results_.push_back({move(name), false, "unknown exception"});
        }
    }

    [[nodiscard]] i32 report(OutputStream &out = cout) const {
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
    Vector<TestResult> results_;
};

template <class A, class B>
void require_equal(const A &actual, const B &expected, StringView expression = "values differ") {
    if (!(actual == expected)) {
        OutputStringStream msg;

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
        static_assert(IsIntegral<Int>::value, "Random::integer requires an integral type");

        return UniformIntegerDistribution<Int>(low, high)(engine_);
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
    RandomEngine engine_;
};

template <class Input, class Oracle, class Candidate, class Format>
bool differential(usize cases, Random &random, Oracle &&oracle, Candidate &&candidate,
                  Format &&format, const Path &failure_file = "counterexample.txt") {
    for (usize i = 0; i < cases; ++i) {
        Input input = invoke(format, random, i);
        const auto expected = invoke(oracle, input);
        const auto actual = invoke(candidate, input);

        if (!(actual == expected)) {
            OutputFileStream out(failure_file, IoState::binary);

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
    const auto start = Clock::now();

    for (usize i = 0; i < iterations; ++i)
        invoke(function, i);
    const f64 elapsed = BasicDuration<f64, MillisecondRatio>(Clock::now() - start).count();

    return {move(name), iterations, elapsed, elapsed * 1'000'000.0 / iterations};
}

inline void print_benchmark(const BenchmarkResult &result, OutputStream &out = cout) {
    out << result.name << ": " << result.iterations << " iterations, " << fixed << setprecision(3)
        << result.totalMilliseconds << " ms total, " << result.nanosecondsPerIteration
        << " ns/iteration\n";
}

} // namespace rangeforge
