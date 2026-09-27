#include <rangeforge/solutions.hpp>

#include <algorithm>
#include <cstdlib>
#include <numeric>

namespace rangeforge::solutions {
namespace {

using std::abs;
using std::gcd;
using std::min;
using std::nth_element;

void or_shift_left(Vector<u64> &bits, usize shift, usize source_bits) {
    if (source_bits == 0)
        return;
    const usize word_shift = shift >> 6;
    const u32 bit_shift = static_cast<u32>(shift & 63);
    usize word = (source_bits - 1) >> 6;
    const u32 tail_bits = static_cast<u32>(source_bits & 63);

    while (true) {
        u64 value = bits[word];

        if (word == ((source_bits - 1) >> 6) && tail_bits != 0) {
            value &= (u64{1} << tail_bits) - 1;
        }
        if (value != 0) {
            const usize destination = word + word_shift;

            if (destination < bits.size())
                bits[destination] |= value << bit_shift;
            if (bit_shift != 0 && destination + 1 < bits.size()) {
                bits[destination + 1] |= value >> (64 - bit_shift);
            }
        }
        if (word == 0)
            break;
        --word;
    }
}

bool get_bit(const Vector<u64> &bits, usize index) {
    return ((bits[index >> 6] >> (index & 63)) & 1ULL) != 0;
}

} // namespace

i32 maximum_deletions_packed(Vector<i64> values) {
    const i32 n = static_cast<i32>(values.size());

    if (n <= 1)
        return 0;

    Vector<i64> ordered = values;

    nth_element(ordered.begin(), ordered.begin() + n / 2, ordered.end());
    const i64 median = ordered[n / 2];

    i64 divisor = 0;

    for (const i64 value : values)
        divisor = gcd(divisor, abs(value - median));

    if (divisor == 0)
        return n - 1;

    i64 total_sum = 0;
    usize bound = 0;

    for (i64 &value : values) {
        value = (value - median) / divisor;
        total_sum += value;
        bound += static_cast<usize>(abs(value));
    }

    const i64 denominator = gcd(abs(total_sum), static_cast<i64>(n));
    const i32 q = n / static_cast<i32>(denominator);
    const i32 max_k = (n / 2 / q) * q;

    if (max_k == 0)
        return 0;

    const usize width = 2 * bound + 1;
    const usize total_bits = static_cast<usize>(max_k + 1) * width;
    Vector<u64> dp((total_bits + 63) >> 6, 0);

    dp[bound >> 6] |= u64{1} << (bound & 63);

    i32 processed = 0;

    for (const i64 value : values) {
        const i32 max_source_k = min(processed, max_k - 1);
        const usize source_bits = static_cast<usize>(max_source_k + 1) * width;
        const usize shift = static_cast<usize>(static_cast<i64>(width) + value);

        or_shift_left(dp, shift, source_bits);
        ++processed;
    }

    for (i32 k = q; k <= max_k; k += q) {
        const i64 numerator = total_sum * k;

        if (numerator % n != 0)
            continue;
        const i64 target = numerator / n;

        if (target < -static_cast<i64>(bound) || target > static_cast<i64>(bound))
            continue;
        const usize index =
            static_cast<usize>(k) * width + static_cast<usize>(target + static_cast<i64>(bound));

        if (get_bit(dp, index))
            return n - k;
    }
    return 0;
}

} // namespace rangeforge::solutions
