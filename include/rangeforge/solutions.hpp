#pragma once

#include <rangeforge/types.hpp>

namespace rangeforge::solutions {

// Returns the maximum number of deletions that preserve a nonempty mean.
i32 maximum_deletions_balanced(Vector<i32> values);
i32 maximum_deletions_packed(Vector<i64> values);

} // namespace rangeforge::solutions
