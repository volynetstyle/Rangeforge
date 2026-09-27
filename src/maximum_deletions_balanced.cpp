#include <rangeforge/solutions.hpp>

#include <algorithm>
#include <numeric>

namespace rangeforge::solutions {

using std::count_if;
using std::gcd;
using std::max;
using std::min;
using std::min_element;

i32 maximum_deletions_balanced(Vector<i32> values) {
    const i32 n = static_cast<i32>(values.size());
    if (n <= 1)
        return 0;

    const i32 minimum = *min_element(values.begin(), values.end());
    i32 step = 0;
    for (const i32 value : values)
        step = gcd(step, value - minimum);
    if (step == 0)
        return n - 1;

    i32 sum = 0;
    i32 range = 0;
    for (i32 &value : values) {
        value = (value - minimum) / step;
        sum += value;
        range = max(range, value);
    }
    for (const i32 value : values) {
        if (static_cast<i64>(value) * n == sum)
            return n - 1;
    }

    const i32 positive_count =
        static_cast<i32>(count_if(values.begin(), values.end(), [n, sum](i32 value) {
            return static_cast<i64>(value) * n > sum;
        }));
    if (positive_count > n / 2) {
        for (i32 &value : values)
            value = range - value;
        sum = n * range - sum;
    }

    const i32 q = n / gcd(sum, n);
    const i32 allocated_k = (n / 2 / q) * q;
    if (allocated_k == 0)
        return 0;

    Vector<i32> positive;
    Vector<i32> negative;
    for (const i32 value : values) {
        (static_cast<i64>(value) * n > sum ? positive : negative).push_back(value);
    }
    const i32 m = static_cast<i32>(negative.size());
    const u8 inf = static_cast<u8>(m + 1);

    Vector<i32> low(allocated_k + 1);
    Vector<i32> nonpositive_count(allocated_k + 1);
    for (i32 k = 1; k <= allocated_k; ++k) {
        low[k] = static_cast<i32>(static_cast<i64>(k - 1) * sum / n) + 1;
        nonpositive_count[k] = static_cast<i32>(static_cast<i64>(k) * sum / n) - low[k] + 1;
    }

    const usize cells = static_cast<usize>(allocated_k + 1) * static_cast<usize>(range);
    Vector<u8> g(cells, inf);
    Vector<u8> expanded(cells, inf);

    i32 best = n;
    i32 live_k = allocated_k;
    for (const i32 x : positive) {
        for (i32 k = live_k - 1; k >= 1; --k) {
            const u8 *source = g.data() + static_cast<usize>(k) * range;
            u8 *destination = g.data() + static_cast<usize>(k + 1) * range;
            const i32 offset = x + low[k] - low[k + 1];
            for (i32 h = 0; h < nonpositive_count[k]; ++h) {
                destination[h + offset] = min(destination[h + offset], source[h]);
            }
        }

        g[static_cast<usize>(range) + x - low[1]] = 0;

        for (i32 k = 1; k < live_k; ++k) {
            u8 *current = g.data() + static_cast<usize>(k) * range;
            u8 *done = expanded.data() + static_cast<usize>(k) * range;
            u8 *next = g.data() + static_cast<usize>(k + 1) * range;
            const i32 row_offset = low[k] - low[k + 1];
            for (i32 h = nonpositive_count[k]; h < range; ++h) {
                const i32 cursor = current[h];
                const i32 previous = done[h];
                if (cursor >= previous)
                    continue;
                done[h] = static_cast<u8>(cursor);
                const i32 last = min(m, previous);
                for (i32 index = cursor + 1; index <= last; ++index) {
                    const i32 destination = h + negative[index - 1] + row_offset;
                    next[destination] = min(next[destination], static_cast<u8>(index));
                }
            }
        }

        for (i32 k = q; k <= live_k; k += q) {
            const i32 target = static_cast<i32>(static_cast<i64>(k) * sum / n);
            if (g[static_cast<usize>(k) * range + target - low[k]] != inf) {
                best = k;
                live_k = ((best - 1) / q) * q;
                break;
            }
        }
        if (live_k == 0)
            break;
    }
    return n - best;
}

} // namespace rangeforge::solutions
