cmake_minimum_required(VERSION 3.21)
project(rangeforge LANGUAGES CXX)

option(RANGEFORGE_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(RANGEFORGE_ENABLE_SANITIZERS)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    message(FATAL_ERROR "RANGEFORGE_ENABLE_SANITIZERS requires GCC or Clang.")
  endif()
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

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

find_program(RANGEFORGE_LINT_CLANG_EXE NAMES clang REQUIRED)
execute_process(
  COMMAND "${RANGEFORGE_LINT_CLANG_EXE}" -print-resource-dir
  OUTPUT_VARIABLE RANGEFORGE_LINT_RESOURCE_DIRECTORY
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY
)
if(NOT IS_DIRECTORY "${RANGEFORGE_LINT_RESOURCE_DIRECTORY}/include")
  message(FATAL_ERROR "Clang resource headers are missing: ${RANGEFORGE_LINT_RESOURCE_DIRECTORY}")
endif()
string(REPLACE "\\" "\\\\" RANGEFORGE_LINT_RESOURCE_DIRECTORY_CPP
  "${RANGEFORGE_LINT_RESOURCE_DIRECTORY}")
string(REPLACE "\"" "\\\"" RANGEFORGE_LINT_RESOURCE_DIRECTORY_CPP
  "${RANGEFORGE_LINT_RESOURCE_DIRECTORY_CPP}")

set(RANGEFORGE_LINT_TARGET "${CMAKE_CXX_COMPILER_TARGET}")
if(RANGEFORGE_LINT_TARGET STREQUAL "")
  execute_process(
    COMMAND "${CMAKE_CXX_COMPILER}" -dumpmachine
    OUTPUT_VARIABLE RANGEFORGE_LINT_TARGET
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
endif()

set(RANGEFORGE_LINT_IMPLICIT_INCLUDE_INITIALIZERS "")
foreach(RANGEFORGE_LINT_INCLUDE_DIR IN LISTS CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
  file(TO_CMAKE_PATH "${RANGEFORGE_LINT_INCLUDE_DIR}" RANGEFORGE_LINT_INCLUDE_NORMALIZED)
  # libclang needs Clang's intrinsic headers, while retaining GCC's C++ library headers.
  if(RANGEFORGE_LINT_INCLUDE_NORMALIZED MATCHES "/lib/gcc/[^/]+/[^/]+/include(-fixed)?$")
    continue()
  endif()
  string(REPLACE "\\" "\\\\" RANGEFORGE_LINT_INCLUDE_DIR_CPP "${RANGEFORGE_LINT_INCLUDE_DIR}")
  string(REPLACE "\"" "\\\"" RANGEFORGE_LINT_INCLUDE_DIR_CPP "${RANGEFORGE_LINT_INCLUDE_DIR_CPP}")
  if(NOT RANGEFORGE_LINT_IMPLICIT_INCLUDE_INITIALIZERS STREQUAL "")
    string(APPEND RANGEFORGE_LINT_IMPLICIT_INCLUDE_INITIALIZERS ",\n")
  endif()
  string(APPEND RANGEFORGE_LINT_IMPLICIT_INCLUDE_INITIALIZERS
    "    \"${RANGEFORGE_LINT_INCLUDE_DIR_CPP}\"")
endforeach()

string(REPLACE "\\" "\\\\" RANGEFORGE_LINT_TARGET_CPP "${RANGEFORGE_LINT_TARGET}")
string(REPLACE "\"" "\\\"" RANGEFORGE_LINT_TARGET_CPP "${RANGEFORGE_LINT_TARGET_CPP}")
string(REPLACE "\\" "\\\\" RANGEFORGE_LINT_GENERATED_INCLUDE_CPP
  "${CMAKE_CURRENT_BINARY_DIR}/generated")
string(REPLACE "\"" "\\\"" RANGEFORGE_LINT_GENERATED_INCLUDE_CPP
  "${RANGEFORGE_LINT_GENERATED_INCLUDE_CPP}")
set(RANGEFORGE_LINT_SYSROOT "${CMAKE_SYSROOT}")
if(RANGEFORGE_LINT_SYSROOT STREQUAL "" AND APPLE)
  set(RANGEFORGE_LINT_SYSROOT "${CMAKE_OSX_SYSROOT}")
endif()
string(REPLACE "\\" "\\\\" RANGEFORGE_LINT_SYSROOT_CPP "${RANGEFORGE_LINT_SYSROOT}")
string(REPLACE "\"" "\\\"" RANGEFORGE_LINT_SYSROOT_CPP "${RANGEFORGE_LINT_SYSROOT_CPP}")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(
  tools/lint/BuildConfig.hpp.in
  "${CMAKE_CURRENT_BINARY_DIR}/generated/BuildConfig.hpp"
  @ONLY
)

add_executable(rangeforge-lint
  tools/lint/main.cpp
  tools/lint/ClangApi.cpp
  tools/lint/SeparateStatementGroups.cpp
)
target_compile_features(rangeforge-lint PRIVATE cxx_std_20)
target_include_directories(rangeforge-lint PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/generated"
  "${CMAKE_CURRENT_SOURCE_DIR}/tools/lint"
)
if(CMAKE_DL_LIBS)
  target_link_libraries(rangeforge-lint PRIVATE ${CMAKE_DL_LIBS})
endif()

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
