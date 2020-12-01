#pragma once

#include "b6502/base.h"
#include "b6502/component.h"
#include "b6502/reset_manager.h"

/**
 *  @brief A struct for the NES' picture processing unit (PPU).
 */
typedef struct PPU {
  struct Component;
} PPU;

/**
 *  @brief Constructor for the PPU
 */
PPU* ppu_create(ResetManager* rm);
