#pragma once

/// @file bus.h

#include "b6502/base.h"

#define NUMBER_OF_PAGES (size_t)256

/**
 *  @brief A struct for the communication bus.
 */
typedef struct {
  void* handlers[NUMBER_OF_PAGES];
} Bus;

/**
 * @brief Add a component to the communication bus.
 * @param bus A pointer to the communication bus.
 * @param obj A pointer to the component.
 * @param start The start address of where to begin mapping the component.
 * @param end The end address of where to stop mapping the component.
 */
void map_handler(Bus* bus, void* obj, uint16_t start, uint16_t end);

/**
 * @brief Request a read from the communication bus.
 * @param bus A pointer to the communication bus.
 * @param addr The address to read one byte from.
 * @return The byte that was read.
 */
uint8_t read(Bus* bus, uint16_t addr);

/**
 * @brief Request a write to the communication bus.
 * @param bus A pointer to the communication bus.
 * @param addr The address to write the byte.
 * @param val The byte to write.
 */
void write(Bus* bus, uint16_t addr, uint8_t val);
