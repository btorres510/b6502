#include "b6502/nes/ppu.h"

#include "b6502/rc.h"
#include "b6502/reset_manager.h"

static void deinit(void* UNUSED(obj)) {}

static void ppu_reset(void* UNUSED(obj)) {}

PPU* ppu_create(ResetManager* rm) {
  PPU* ppu = rc_alloc(sizeof(*ppu), deinit);
  add_rm_device(rm, ppu, ppu_reset);
  return ppu;
}
