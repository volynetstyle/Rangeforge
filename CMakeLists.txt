cmake_minimum_required(VERSION 3.28)
project(rangeforge LANGUAGES CXX)

include(CTest)
include(cmake/ProjectOptions.cmake)

add_subdirectory(include)
add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
