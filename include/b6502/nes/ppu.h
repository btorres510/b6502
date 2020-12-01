#pragma once

#include "b6502/base.h"
#include "b6502/component.h"
#include "b6502/reset_manager.h"

typedef struct PPU {
  struct Component;
} PPU;

PPU* ppu_create(ResetManager* rm);
