option(RANGEFORGE_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(RANGEFORGE_ENABLE_CLANG_TIDY "Run clang-tidy during compilation" OFF)

add_library(rangeforge_project_options INTERFACE)

if(RANGEFORGE_ENABLE_SANITIZERS)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    message(FATAL_ERROR "RANGEFORGE_ENABLE_SANITIZERS requires GCC or Clang.")
  endif()
  target_compile_options(rangeforge_project_options INTERFACE
    -fsanitize=address,undefined -fno-omit-frame-pointer
  )
  target_link_options(rangeforge_project_options INTERFACE -fsanitize=address,undefined)
endif()

if(RANGEFORGE_ENABLE_CLANG_TIDY)
  find_program(RANGEFORGE_CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
endif()

function(rangeforge_apply_project_options target)
  target_link_libraries(${target} PRIVATE rangeforge_project_options)
  if(RANGEFORGE_ENABLE_CLANG_TIDY)
    set_property(TARGET ${target} PROPERTY CXX_CLANG_TIDY
      "${RANGEFORGE_CLANG_TIDY_EXE};--config-file=${PROJECT_SOURCE_DIR}/.clang-tidy"
    )
  endif()
endfunction()
