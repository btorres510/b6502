function(set_project_warnings project_name)
  set(MSVC_WARNINGS /W4 /Wall /permissive-)

  set(CLANG_WARNINGS
      -Weverything
      -Wno-padded
      -Wno-c99-compat
      -Wno-switch-enum
      -Wno-nested-anon-types
      -Wno-format-nonliteral
      -Wno-deprecated-register
      -Wno-gnu-anonymous-struct
      -Wno-signed-enum-bitfield 
      -Wno-poison-system-directories
      -Wno-documentation-unknown-command
      -Wno-gnu-zero-variadic-macro-arguments
  )

  # https://developers.redhat.com/blog/2018/03/21/compiler-and-linker-flags-gcc/
  set(GCC_WARNINGS
      -Wall
      -Wextra
      -Wshadow
      -Wunused
      -Wpedantic
      -Wformat=2
      -Winit-self
      -Wmultichar
      -Wcast-align
      -Wconversion
      -Wlogical-op
      -Wparentheses
      -Wtype-limits
      -Wwrite-strings
      -Wpointer-arith
      -Wduplicated-cond
      -Wformat-security
      -Wsign-conversion
      -Waggregate-return
      -Wnull-dereference
      -Wunreachable-code
      -Wdouble-promotion
      -Wformat-nonliteral
      -Wduplicated-branches
      -Wmissing-declarations
      -Wmissing-include-dirs
      -Wmisleading-indentation
      -Werror=implicit-function-declaration
  )

  if(MSVC)
    set(PROJECT_WARNINGS ${MSVC_WARNINGS})
  elseif(CMAKE_C_COMPILER_ID MATCHES ".*Clang")
    set(PROJECT_WARNINGS ${CLANG_WARNINGS})
  elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(PROJECT_WARNINGS ${GCC_WARNINGS})
  else()
    message(AUTHOR_WARNING "No compiler warnings set for '${CMAKE_C_COMPILER_ID}' compiler.")
  endif()

  target_compile_options(${project_name} PUBLIC ${PROJECT_WARNINGS})

  if(NOT TARGET ${project_name})
    message(AUTHOR_WARNING "${project_name} is not a target, thus no compiler warnings were added.")
  endif()
endfunction()
