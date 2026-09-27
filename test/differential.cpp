#include <rangeforge/solutions.hpp>
#include <rangeforge/subset_average_oracle.hpp>
#include <rangeforge/test.hpp>

namespace rf = rangeforge;

int main() {
    rf::TestRunner tests;
    const auto check_against_oracle = [](const rf::Vector<rf::i32>& values) {
        const rf::i32 expected = rf::subset_average_oracle(values);
        rf::require_equal(rf::solutions::maximum_deletions_balanced(values), expected,
                          "balanced implementation versus source of truth");
        const rf::Vector<rf::i64> wide(values.begin(), values.end());
        rf::require_equal(rf::solutions::maximum_deletions_packed(wide), expected,
                          "packed implementation versus source of truth");
    };

    tests.test("single element", [&] { check_against_oracle({5}); });
    tests.test("all equal", [&] { check_against_oracle({7, 7, 7, 7}); });
    tests.test("negative and positive", [&] { check_against_oracle({-2, 0, 2}); });
    tests.test("no proper mean subset", [&] { check_against_oracle({1, 2, 4}); });
    tests.test("duplicate mean values", [&] { check_against_oracle({0, 0, 3, 3}); });
    tests.test("empty input", [&] { check_against_oracle({}); });

    tests.test("exhaustive small arrays", [&] {
        for (rf::i32 n = 1; n <= 8; ++n) {
            rf::u64 combinations = 1;
            for (rf::i32 i = 0; i < n; ++i) combinations *= 5;
            for (rf::u64 code = 0; code < combinations; ++code) {
                rf::u64 digits = code;
                rf::Vector<rf::i32> values(n);
                for (rf::i32& value : values) {
                    value = static_cast<rf::i32>(digits % 5) - 2;
                    digits /= 5;
                }
                check_against_oracle(values);
            }
        }
    });

    tests.test("seeded random differential", [&] {
        rf::Random random(0xD1FF2026);
        for (rf::usize case_index = 0; case_index < 5000; ++case_index) {
            const rf::usize n = random.integer<rf::usize>(1, 14);
            const auto values = random.integers<rf::Vector<rf::i32>>(n, -30, 30);
            const rf::i32 expected = rf::subset_average_oracle(values);
            const rf::i32 balanced = rf::solutions::maximum_deletions_balanced(values);
            const rf::Vector<rf::i64> wide(values.begin(), values.end());
            const rf::i32 packed = rf::solutions::maximum_deletions_packed(wide);
            if (balanced != expected || packed != expected) {
                rf::OutputFileStream counterexample("counterexample.txt");
                counterexample << "case " << case_index << " seed " << random.seed() << '\n';
                for (const auto value : values) counterexample << value << ' ';
                counterexample << '\n';
                rf::require_equal(balanced, expected, "balanced random case");
                rf::require_equal(packed, expected, "packed random case");
            }
        }
    });

    return tests.report();
}
