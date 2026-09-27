#include <rangeforge/subset_average_oracle.hpp>
#include <rangeforge/test.hpp>
#include <rangeforge/types.hpp>

#include <vector>

namespace rf = rangeforge;

// Replace this deliberately simple candidate with the optimized implementation.
rf::i32 candidate(const rf::Vec<rf::i32> &values) { return rf::subset_average_oracle(values); }

int main() {
    rf::TestRunner tests;
    tests.test("empty input", [] { rf::require_equal(rf::subset_average_oracle({}), 0); });
    tests.test("all equal", [] { rf::require_equal(rf::subset_average_oracle({7, 7, 7}), 2); });
    tests.test("negative values",
               [] { rf::require_equal(rf::subset_average_oracle({-2, 0, 2}), 2); });
    if (tests.report() != 0)
        return 1;

    rf::Random random(123456789);
    const bool matched = rf::differential<rf::Vec<rf::i32>>(
        1000, random, rf::subset_average_oracle, candidate, [](rf::Random &rng, rf::usize) {
            return rng.integers<rf::Vec<rf::i32>>(rng.integer<rf::i32>(1, 12), -20, 20);
        });
    rf::require(matched, "differential mismatch; see counterexample.txt");

    auto result = rf::benchmark("subset-average oracle", 100, [](rf::usize i) {
        volatile rf::i32 sink = rf::subset_average_oracle({rf::i32(i % 9), 4, -2, 7, 3});
        (void)sink;
    });
    rf::print_benchmark(result);
}
