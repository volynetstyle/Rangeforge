#include <rangeforge/subset_average_oracle.hpp>
#include <rangeforge/test.hpp>

#include "oracle_test_support.hpp"

namespace rf = rangeforge;

int main() {
    rf::TestRunner tests;

    tests.test("oracle agrees with exhaustive subset enumeration", [] {
        for (rf::i32 n = 0; n <= 8; ++n) {
            rf::u64 combinations = 1;

            for (rf::i32 index = 0; index < n; ++index)
                combinations *= 5;
            for (rf::u64 code = 0; code < combinations; ++code) {
                rf::u64 digits = code;
                rf::Vector<rf::i32> values(n);

                for (rf::i32 &value : values) {
                    value = static_cast<rf::i32>(digits % 5) - 2;
                    digits /= 5;
                }
                rf::require_equal(rf::subset_average_oracle(values),
                                  oracle_test::brute_force(values),
                                  "oracle versus exhaustive reference");
            }
        }
    });

    tests.test("oracle agrees with seeded random reference", [] {
        rf::Random random(0x0A11CE);

        for (rf::usize case_index = 0; case_index < 5000; ++case_index) {
            const rf::usize n = random.integer<rf::usize>(0, 14);
            const auto values = random.integers<rf::Vector<rf::i32>>(n, -100, 100);

            rf::require_equal(rf::subset_average_oracle(values), oracle_test::brute_force(values),
                              "oracle versus random reference");
        }
    });

    tests.test("oracle rejects inputs outside its contract", [] {
        bool too_many_rejected = false;

        try {
            (void)rf::subset_average_oracle(rf::Vector<rf::i32>(51, 0));
        } catch (const rf::InvalidArgument &) {
            too_many_rejected = true;
        }
        rf::require(too_many_rejected, "n > 50 must be rejected");

        bool value_rejected = false;

        try {
            (void)rf::subset_average_oracle({10001});
        } catch (const rf::InvalidArgument &) {
            value_rejected = true;
        }
        rf::require(value_rejected, "out-of-range value must be rejected");
    });

    return tests.report();
}
