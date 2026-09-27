#pragma once

#include <algorithm>
#include <bit>
#include <numeric>
#include <cstdlib>

#include <rangeforge/subset_average_oracle.hpp>

namespace oracle_mutants {
using std::abs;
using std::accumulate;
using std::popcount;
using rangeforge::i32;
using rangeforge::i64;
using rangeforge::Vector;

// Mutant: return the smallest valid subset instead of the largest one.
inline i32 returns_first_valid_size(const Vector<i32>& values) {
    const i32 n = static_cast<i32>(values.size());

    if (n <= 1) return 0;
    const i64 total = accumulate(values.begin(), values.end(), i64{0});

    for (i32 wanted = 1; wanted < n; ++wanted) {
        const rangeforge::u64 limit = rangeforge::u64{1} << n;

        for (rangeforge::u64 mask = 1; mask + 1 < limit; ++mask) {
            if (popcount(mask) != static_cast<unsigned>(wanted)) continue;
            i64 sum = 0;

            for (i32 index = 0; index < n; ++index) {
                if ((mask & (rangeforge::u64{1} << index)) != 0) sum += values[index];
            }
            if (sum * n == total * wanted) return wanted;
        }
    }
    return 0;
}

// Mutant: accidentally accepts deleting every item, leaving an empty result.
inline i32 allows_empty_remainder(const Vector<i32>& values) {
    return values.empty() ? 0 : static_cast<i32>(values.size());
}

// Mutant: loses the sign of every input value during normalization.
inline i32 takes_absolute_values(const Vector<i32>& values) {
    Vector<i32> transformed;
    transformed.reserve(values.size());

    for (i32 value : values) transformed.push_back(static_cast<i32>(abs(value)));
    return rangeforge::subset_average_oracle(transformed);
}

// Mutant: truncates a non-integral target mean instead of rejecting that size.
inline i32 truncates_target_sum(const Vector<i32>& values) {
    const i32 n = static_cast<i32>(values.size());

    if (n <= 1) return 0;
    const i64 total = accumulate(values.begin(), values.end(), i64{0});
    const rangeforge::u64 limit = rangeforge::u64{1} << n;

    for (i32 wanted = n - 1; wanted >= 1; --wanted) {
        const i64 target = total * wanted / n;

        for (rangeforge::u64 mask = 1; mask + 1 < limit; ++mask) {
            if (popcount(mask) != static_cast<unsigned>(wanted)) continue;
            i64 sum = 0;

            for (i32 index = 0; index < n; ++index) {
                if ((mask & (rangeforge::u64{1} << index)) != 0) sum += values[index];
            }
            if (sum == target) return wanted;
        }
    }
    return 0;
}

// Mutant: reports the complement size, i.e. the minimum valid deletion count.
inline i32 returns_complement_size(const Vector<i32>& values) {
    const i32 answer = rangeforge::subset_average_oracle(values);

    return answer == 0 ? 0 : static_cast<i32>(values.size()) - answer;
}

} // namespace oracle_mutants
