// Historical note:
// This work started from a solution to the original problem shown to me by
// Dmytro Kalinovskyi (@rt4x) around 2023, when he was a second-year student.
//
// The Rangeforge oracle below is an independent formalization and implementation
// of the problem, using an exact count × subset-sum packed DP.
//
// https://github.com/rt4x
// https://www.linkedin.com/in/dmytro-kalinovskyi-m/
#pragma once

#include <algorithm>
#include <numeric>

#include <rangeforge/types.hpp>

namespace rangeforge {

using std::min;

// Source-of-truth specification:
// Return max |D| over proper subsets D of the input such that deleting D leaves
// a nonempty array with the same arithmetic mean as the original array.
// Equivalently, D itself has the original mean. The empty deletion is allowed,
// so the result is zero when no nonempty proper subset has that mean.
//
// Supported domain: at most 50 values, each in [-10000, 10000]. The dynamic
// packed DP tracks exact subset sums and counts, with sum range bounded by the
// sum of absolute input values.
inline i32 subset_average_oracle(const Vector<i32> &values) {
    const i32 n = static_cast<i32>(values.size());

    if (n == 0)
        return 0;
    if (n > 50)
        throw InvalidArgument("oracle supports at most 50 values");

    i32 sum_bound = 0;
    i64 total = 0;

    for (const i32 value : values) {
        if (value < -10000 || value > 10000) {
            throw InvalidArgument("oracle values must be in [-10000, 10000]");
        }

        sum_bound += value < 0 ? -value : value;
        total += value;
    }

    const usize width = static_cast<usize>(2 * sum_bound + 1);
    const usize word_count = (width + 63) / 64;
    Vector<Vector<u64>> reachable(n, Vector<u64>(word_count, 0));
    const usize zero_index = static_cast<usize>(sum_bound);

    reachable[0][zero_index >> 6] |= u64{1} << (zero_index & 63);

    i32 processed = 0;

    for (const i32 value : values) {
        const i32 max_count = min(n - 1, processed + 1);

        for (i32 count = max_count; count >= 1; --count) {
            const Vector<u64> &source = reachable[count - 1];
            Vector<u64> &destination = reachable[count];

            if (value >= 0) {
                const usize word_shift = static_cast<usize>(value) >> 6;
                const u32 bit_shift = static_cast<u32>(value) & 63;

                for (usize word = 0; word < word_count; ++word) {
                    const u64 bits = source[word];
                    const usize target = word + word_shift;

                    if (target < word_count)
                        destination[target] |= bits << bit_shift;

                    if (bit_shift != 0 && target + 1 < word_count) {
                        destination[target + 1] |= bits >> (64 - bit_shift);
                    }
                }
            } else {
                const usize magnitude = static_cast<usize>(-value);
                const usize word_shift = magnitude >> 6;
                const u32 bit_shift = static_cast<u32>(magnitude) & 63;

                for (usize word = word_shift; word < word_count; ++word) {
                    const u64 bits = source[word];
                    const usize target = word - word_shift;
                    destination[target] |= bits >> bit_shift;

                    if (bit_shift != 0 && target > 0) {
                        destination[target - 1] |= bits << (64 - bit_shift);
                    }
                }
            }
            const u32 tail_bits = static_cast<u32>(width & 63);

            if (tail_bits != 0) {
                destination.back() &= (u64{1} << tail_bits) - 1;
            }
        }
        ++processed;
    }

    for (i32 count = n - 1; count >= 1; --count) {
        const i64 numerator = total * count;

        if (numerator % n != 0)
            continue;

        const i64 target_sum = numerator / n;

        if (target_sum < -sum_bound || target_sum > sum_bound)
            continue;

        const usize index = static_cast<usize>(target_sum + sum_bound);

        if (((reachable[count][index >> 6] >> (index & 63)) & 1ULL) != 0)
            return count;
    }
    return 0;
}

} // namespace rangeforge
