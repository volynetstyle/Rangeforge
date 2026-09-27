add_library(rangeforge INTERFACE)
add_library(rangeforge::rangeforge ALIAS rangeforge)

target_compile_features(rangeforge INTERFACE cxx_std_20)
target_sources(rangeforge INTERFACE
  FILE_SET HEADERS
  BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
  FILES
    rangeforge/solutions.hpp
    rangeforge/subset_average_oracle.hpp
    rangeforge/test.hpp
    rangeforge/types.hpp
)
