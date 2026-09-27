#pragma once

#include <rangeforge/types.hpp>

namespace rangeforge::solutions {

// Returns the maximum number of deletions that preserve a nonempty mean.
i32 maximum_deletions_balanced(Vec<i32> values);
i32 maximum_deletions_packed(Vec<i64> values);

} // namespace rangeforge::solutions
