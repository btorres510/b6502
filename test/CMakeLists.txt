cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(b6502Tests LANGUAGES C)

# ---- Options ----

option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)
option(TEST_INSTALLED_VERSION "Test the version found by find_package" OFF)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)

if(TEST_INSTALLED_VERSION)
  find_package(b6502 REQUIRED)
else()
  CPMAddPackage(NAME b6502 SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
endif()

CPMAddPackage(
  NAME Format.cmake
  GITHUB_REPOSITORY TheLartians/Format.cmake
  VERSION 1.6
  OPTIONS # enable cmake formatting
          "FORMAT_CHECK_CMAKE ON"
)

# For some reason, unity extras aren't included in unity's CMakeLists.txt. I am using gmircea's pull
# request that adds these extras.
CPMAddPackage(
  NAME unity
  GITHUB_REPOSITORY gmircea/Unity
  VERSION 2.5.1
  GIT_TAG 6238bc66d16b4d02ca3a1581148234cc722fe469
  OPTIONS # Add unity extras
          "WITH_ADDONS On"
)

# ---- Create binary ----
include(../cmake/SourcesAndHeaders.cmake)

add_executable(${PROJECT_NAME} ${test_sources})
target_link_libraries(${PROJECT_NAME} PRIVATE b6502 unity::framework)
target_compile_features(${PROJECT_NAME} PUBLIC c_std_11)

# ---- Add b6502Tests ----

enable_testing()

add_test(${PROJECT_NAME} ${PROJECT_NAME})

# ---- code coverage ----

if(ENABLE_TEST_COVERAGE)
  target_compile_options(b6502 PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
  target_link_options(b6502 PUBLIC -fprofile-arcs -ftest-coverage)
endif()
