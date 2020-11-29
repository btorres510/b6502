#pragma once

/**
 * @file base.h
 * @brief Utilities for every source file.
 *
 * This file provides logging, string coloring, and other helper functions.
 * All logs are flushed to either stdout or stderr. To use the coloring macros,
 * do not use the COLORED() macro, instead use of the predefined color macros (i.e. BLACK())
 * In addition to coloring, you can bold/unbold a string. It is possible to bold a colored string.
 * Other macros include UNUSED() and UNUSED_FUNCTION(), which indicate an unused variable or
 * function. All credit for these macros go to: https://stackoverflow.com/a/12891181
 * See the code sample below for how to use these macros.
 *
 * @code{.c}
 * const char * greeting = "Hello!"
 * LOG_INFO("%s", greeting);
 * BOLD(BLUE("%s", greeting)); // Bold a colored string
 *
 * void function(const char * UNUSED(parameter)) {}
 * @endcode
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Macros from the Linux Kernel that help with branch prediction.
 * @param x The conditional statement.
 */
#ifdef __GNUC__
#  define LIKELY(x) __builtin_expect((x), 1)
#  define UNLIKELY(x) __builtin_expect((x), 0)
#else
#  define LIKELY(x) (x)
#  define UNLIKELY(x) (x)
#endif

/**
 * @brief Helper macro to add color to a string.
 * @param color The color.
 * @param str The string.
 */
#define COLORED(color, str) "\x1B[" #color "m" str "\x1B[39m"
#define BLACK(str) COLORED(30, str)
#define RED(str) COLORED(31, str)
#define GREEN(str) COLORED(32, str)
#define YELLOW(str) COLORED(33, str)
#define BLUE(str) COLORED(34, str)
#define MAGENTA(str) COLORED(35, str)
#define CYAN(str) COLORED(36, str)
#define WHITE(str) COLORED(37, str)
#define BRBLACK(str) COLORED(90, str)
#define BRRED(str) COLORED(91, str)
#define BGREEN(str) COLORED(92, str)
#define BRYELLOW(str) COLORED(93, str)
#define BRBLUE(str) COLORED(94, str)
#define BRMAGENTA(str) COLORED(95, str)
#define BRCYAN(str) COLORED(96, str)
#define BRWHTIE(str) COLORED(97, str)
#define BOLD(str) "\x1B[1m" str "\x1B[22m"
#define UNBOLD(str) "\x1B[22m" str "\x1B[1m"

/**
 * @brief Logger for debug events.
 * @param fmt A debug message.
 *
 * This macro relies on the preprocessor variable DEBUG. Since this project is built
 * using CMake, you can set the preprocessor variable by adding in the root CMakeLists.txt:
 * target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)
 */
#ifdef DEBUG
#  define LOG_DEBUG(fmt, ...)                                                               \
    do {                                                                                    \
      fprintf(stdout, "[" BLUE("DEBUG") "] %s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, \
              ##__VA_ARGS__);                                                               \
    } while (0)
#else
#  define LOG_DEBUG(fmt, ...) \
    do {                      \
    } while (0)
#endif

/**
 * @brief Logger for info events.
 * @param fmt An info message.
 */
#define LOG_INFO(fmt, ...)                                                                \
  do {                                                                                    \
    fprintf(stdout, "[" GREEN("INFO") "] %s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, \
            ##__VA_ARGS__);                                                               \
  } while (0)

/**
 * @brief Logger for error events.
 * @param fmt An error message.
 */
#define LOG_ERROR(fmt, ...)                                                              \
  do {                                                                                   \
    fprintf(stderr, "[" RED("ERROR") "] %s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, \
            ##__VA_ARGS__);                                                              \
  } while (0)

/**
 * @brief Indicate that a variable is possibly unused.
 * @param x The unused variable.
 */
#ifdef __GNUC__
#  define UNUSED(x) UNUSED_##x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_##x
#endif

/**
 * @brief Indicate that a function is possibly unused.
 * @param x The unused function.
 */
#ifdef __GNUC__
#  define UNUSED_FUNCTION(x) __attribute__((__unused__)) UNUSED_##x
#else
#  define UNUSED_FUNCTION(x) UNUSED_##x
#endif

/**
 * @brief A dummy function that can be used as a reset/write function for read-only devices.
 * @param obj A pointer to an object.
 */
inline void dummy(void* obj);

/**
 * @brief Read the contents of a ROM into a memory device.
 * @param path The filesystem path of the ROM.
 * @param dest The destination.
 * @param size The size in bytes of each element.
 * @param nmemb The number of elements to read.
 */
int read_rom(const char* path, void* restrict dest, size_t size, size_t nmemb);
