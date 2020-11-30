#pragma once

/// @file component.h

#include <stdbool.h>

#include "b6502/base.h"

/**
 *  @brief A function variable that points to the read function of a bus component
 */
typedef uint8_t (*read_handler)(void *, uint16_t addr);

/**
 *  @brief A function variable that points to the write function of a bus component
 */
typedef void (*write_handler)(void *, uint16_t addr, uint8_t val);

/**
 *  @brief A generic struct for devices on the communication bus.
 */
typedef struct Component {
  read_handler read;
  write_handler write;
} Component;
