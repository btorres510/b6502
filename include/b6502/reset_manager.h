#pragma once

/// @file reset_manager.h

#include "b6502/base.h"

#define MAX_DEVICES 30

/**
 *  @brief A function variable that points to the reset function of a bus component
 */
typedef void (*reset_handler)(void*);

/**
 *  @brief A struct that keeps a list of devices to reset
 */
typedef struct ResetManager {
  size_t num_devices;
  struct {
    void* obj;
    reset_handler reset;
  } devices[MAX_DEVICES];
} ResetManager;

/**
 *  @brief Constructor for a reset manager.
 */
ResetManager* reset_manager_create(void);

/**
 *  @brief Add a device to the reset manager.
 */
void add_rm_device(ResetManager* rm, void* obj, reset_handler reset);

/**
 *  @brief Send a reset signal that will reset all devices in the list.
 */
void reset_devices(ResetManager* rm);
