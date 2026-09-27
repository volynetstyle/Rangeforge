cmake_minimum_required(VERSION 3.21)
project(rangeforge LANGUAGES CXX)

option(RANGEFORGE_ENABLE_CLANG_TIDY "Run clang-tidy during compilation" OFF)
if(RANGEFORGE_ENABLE_CLANG_TIDY)
  find_program(RANGEFORGE_CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
  set(CMAKE_CXX_CLANG_TIDY "${RANGEFORGE_CLANG_TIDY_EXE};--config-file=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy")
endif()

add_library(rangeforge INTERFACE)
add_library(rangeforge::rangeforge ALIAS rangeforge)
target_include_directories(rangeforge INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(rangeforge INTERFACE cxx_std_20)

add_library(rangeforge_solutions STATIC
  src/maximum_deletions_balanced.cpp
  src/maximum_deletions_packed.cpp
)
add_library(rangeforge::solutions ALIAS rangeforge_solutions)
target_link_libraries(rangeforge_solutions PUBLIC rangeforge::rangeforge)

option(RANGEFORGE_BUILD_TESTS "Build differential tests" ON)
if(RANGEFORGE_BUILD_TESTS)
  enable_testing()
  add_executable(rangeforge_differential_test test/differential.cpp)
  target_link_libraries(rangeforge_differential_test PRIVATE rangeforge::solutions)
  add_test(NAME rangeforge_differential COMMAND rangeforge_differential_test)

  add_executable(rangeforge_oracle_test test/oracle.cpp)
  target_link_libraries(rangeforge_oracle_test PRIVATE rangeforge::rangeforge)
  add_test(NAME rangeforge_oracle COMMAND rangeforge_oracle_test)

  add_executable(rangeforge_mutation_test test/mutation_test.cpp)
  target_link_libraries(rangeforge_mutation_test PRIVATE rangeforge::rangeforge)
  add_test(NAME rangeforge_mutation COMMAND rangeforge_mutation_test)
endif()
