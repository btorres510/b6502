cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(b6502Standalone LANGUAGES C)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)

CPMAddPackage(NAME b6502 SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

# ---- Create standalone executable ----
include(../cmake/SourcesAndHeaders.cmake)
add_executable(${PROJECT_NAME} ${standalone})

set_target_properties(${PROJECT_NAME} PROPERTIES C_STANDARD 11 OUTPUT_NAME "b6502")
target_link_libraries(${PROJECT_NAME} PUBLIC b6502)
