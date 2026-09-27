file(GLOB_RECURSE RANGEFORGE_CPP_FILES
  "${CMAKE_CURRENT_LIST_DIR}/../src/*.c"
  "${CMAKE_CURRENT_LIST_DIR}/../src/*.cc"
  "${CMAKE_CURRENT_LIST_DIR}/../src/*.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../src/*.cxx"
  "${CMAKE_CURRENT_LIST_DIR}/../src/*.h"
  "${CMAKE_CURRENT_LIST_DIR}/../src/*.hh"
  "${CMAKE_CURRENT_LIST_DIR}/../src/*.hpp"
  "${CMAKE_CURRENT_LIST_DIR}/../src/*.hxx"
  "${CMAKE_CURRENT_LIST_DIR}/../test/*.c"
  "${CMAKE_CURRENT_LIST_DIR}/../test/*.cc"
  "${CMAKE_CURRENT_LIST_DIR}/../test/*.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../test/*.cxx"
  "${CMAKE_CURRENT_LIST_DIR}/../test/*.h"
  "${CMAKE_CURRENT_LIST_DIR}/../test/*.hh"
  "${CMAKE_CURRENT_LIST_DIR}/../test/*.hpp"
  "${CMAKE_CURRENT_LIST_DIR}/../test/*.hxx"
  "${CMAKE_CURRENT_LIST_DIR}/../include/*.c"
  "${CMAKE_CURRENT_LIST_DIR}/../include/*.cc"
  "${CMAKE_CURRENT_LIST_DIR}/../include/*.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../include/*.cxx"
  "${CMAKE_CURRENT_LIST_DIR}/../include/*.h"
  "${CMAKE_CURRENT_LIST_DIR}/../include/*.hh"
  "${CMAKE_CURRENT_LIST_DIR}/../include/*.hpp"
  "${CMAKE_CURRENT_LIST_DIR}/../include/*.hxx"
  "${CMAKE_CURRENT_LIST_DIR}/../examples/*.c"
  "${CMAKE_CURRENT_LIST_DIR}/../examples/*.cc"
  "${CMAKE_CURRENT_LIST_DIR}/../examples/*.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/../examples/*.cxx"
  "${CMAKE_CURRENT_LIST_DIR}/../examples/*.h"
  "${CMAKE_CURRENT_LIST_DIR}/../examples/*.hh"
  "${CMAKE_CURRENT_LIST_DIR}/../examples/*.hpp"
  "${CMAKE_CURRENT_LIST_DIR}/../examples/*.hxx"
  "${CMAKE_CURRENT_LIST_DIR}/*.c"
  "${CMAKE_CURRENT_LIST_DIR}/*.cc"
  "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/*.cxx"
  "${CMAKE_CURRENT_LIST_DIR}/*.h"
  "${CMAKE_CURRENT_LIST_DIR}/*.hh"
  "${CMAKE_CURRENT_LIST_DIR}/*.hpp"
  "${CMAKE_CURRENT_LIST_DIR}/*.hxx"
)

if(NOT RANGEFORGE_CPP_FILES)
  message(FATAL_ERROR "No C++ source files found to format.")
endif()

execute_process(
  COMMAND clang-format -i ${RANGEFORGE_CPP_FILES}
  RESULT_VARIABLE RANGEFORGE_FORMAT_RESULT
)
if(NOT RANGEFORGE_FORMAT_RESULT EQUAL 0)
  message(FATAL_ERROR "clang-format failed.")
endif()
