add_library(rangeforge_solutions STATIC
  maximum_deletions_balanced.cpp
  maximum_deletions_packed.cpp
)
add_library(rangeforge::solutions ALIAS rangeforge_solutions)

target_link_libraries(rangeforge_solutions PUBLIC rangeforge::rangeforge)
rangeforge_apply_project_options(rangeforge_solutions)
