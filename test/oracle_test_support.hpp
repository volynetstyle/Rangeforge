#pragma once

#include <algorithm>
#include <numeric>

#include <rangeforge/subset_average_oracle.hpp>

namespace oracle_test {

using std::accumulate;
using std::max;

inline rangeforge::i32 brute_force(const rangeforge::Vector<rangeforge::i32>& values) {
    using namespace rangeforge;
    const i32 n = static_cast<i32>(values.size());

    if (n <= 1) return 0;
    const i64 total = accumulate(values.begin(), values.end(), i64{0});
    i32 best = 0;
    const u64 limit = u64{1} << n;

    for (u64 mask = 1; mask + 1 < limit; ++mask) {
        i64 subset_sum = 0;
        i32 count = 0;

        for (i32 index = 0; index < n; ++index) {
            if ((mask & (u64{1} << index)) != 0) {
                subset_sum += values[index];
                ++count;
            }
        }
        if (subset_sum * n == total * count) best = max(best, count);
    }
    return best;
}

} // namespace oracle_test
