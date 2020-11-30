#pragma once

/// @file memory.h

#include "b6502/base.h"
#include "b6502/component.h"

/**
 *  @brief A generic struct for memory devices (RAM, ROM, etc.)
 */
typedef struct {
  struct Component;
  size_t size;
  uint8_t* bytes;
} Memory;

/**
 * @brief A constructor for memory devices.
 * @param size The size of the memory device.
 * @return The memory device that was created
 */
Memory* memory_create(size_t size, read_handler read, write_handler write);

/**
 * @brief A constructor for generic memory devices.
 * @param size The size of the memory device.
 * @return The memory device that was created
 */
Memory* memory_generic_create(size_t size);

/**
 * @brief A generic read function for memory devices
 * @param obj A pointer to the memory device.
 * @param addr The address to read one byte from
 * @return The byte that was read
 */
uint8_t generic_read(void* obj, uint16_t addr);

/**
 * @brief A generic write function for memory devices
 * @param obj A pointer to the memory device.
 * @param addr The address to write one byte to
 * @param val The byte to write
 */
void generic_write(void* obj, uint16_t addr, uint8_t val);

/**
 * @brief A generic reset function for memory devices
 * @param obj A pointer to the memory device.
 */
void generic_reset(void* obj);
