#pragma once

/// @file reset_manager.h

#include "b6502/base.h"

#define MAX_DEVICES 30

/**
 *  @brief A function variable that points to the reset function of a bus component
 */
typedef void (*reset_handler)(void*);

typedef struct ResetManager {
  size_t num_devices;
  struct {
    void* obj;
    reset_handler reset;
  } devices[MAX_DEVICES];
} ResetManager;

ResetManager* reset_manager_create(void);
void add_rm_device(ResetManager* rm, void* obj, reset_handler reset);
void reset(ResetManager* rm);
