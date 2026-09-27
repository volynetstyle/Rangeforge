#include <rangeforge/test.hpp>

#include "oracle_test_support.hpp"
#include "mutants/oracle_mutants.hpp"

namespace rf = rangeforge;

int main() {
    using std::to_string;
    rf::TestRunner tests;

    tests.test("mutation suite kills all oracle mutants", [] {
        using Mutant = rf::i32 (*)(const rf::Vector<rf::i32>&);
        const rf::Array<Mutant, 5> mutants = {
            oracle_mutants::returns_first_valid_size,
            oracle_mutants::allows_empty_remainder,
            oracle_mutants::takes_absolute_values,
            oracle_mutants::truncates_target_sum,
            oracle_mutants::returns_complement_size,
        };
        const rf::Array<rf::Vector<rf::i32>, 5> witnesses = {
            rf::Vector<rf::i32>{5, 5, 5, 5},
            rf::Vector<rf::i32>{1, 2, 4},
            rf::Vector<rf::i32>{-4, 4},
            rf::Vector<rf::i32>{0, 0, 0, 1},
            rf::Vector<rf::i32>{5, 5, 5, 5},
        };

        for (rf::usize index = 0; index < mutants.size(); ++index) {
            const rf::i32 expected = oracle_test::brute_force(witnesses[index]);
            const rf::i32 mutated = mutants[index](witnesses[index]);

            rf::require(mutated != expected, "surviving mutant at index " + to_string(index));
        }
    });
    return tests.report();
}
