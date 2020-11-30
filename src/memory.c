#include "b6502/memory.h"

#include <assert.h>
#include <stdlib.h>

#include "b6502/rc.h"
#include "b6502/reset_manager.h"

static void deinit(void* obj) {
  Memory* mem = obj;
  rc_strong_release((void*)&mem->bytes);
}

Memory* rom_create(size_t size, read_handler read) {
  Memory* mem = rc_alloc(sizeof(*mem), deinit);
  mem->bytes = calloc(size, sizeof(*mem->bytes));
  mem->read = read;
  mem->write = NULL;
  return mem;
}

Memory* ram_create(ResetManager* rm, size_t size, read_handler read, write_handler write,
                   reset_handler reset) {
  Memory* mem = rc_alloc(sizeof(*mem), deinit);
  mem->bytes = calloc(size, sizeof(*mem->bytes));
  mem->size = size;
  mem->read = read;
  mem->write = write;
  add_rm_device(rm, mem, reset);

  return mem;
}

Memory* memory_generic_create(ResetManager* rm, size_t size) {
  Memory* mem = rc_alloc(sizeof(*mem), deinit);
  mem->bytes = rc_alloc(size * sizeof(*mem->bytes), NULL);
  mem->size = size;
  mem->read = generic_read;
  mem->write = generic_write;
  add_rm_device(rm, mem, generic_reset);

  return mem;
}

uint8_t generic_read(void* obj, uint16_t addr) {
  Memory* mem = obj;
  assert(addr < mem->size);
  return mem->bytes[addr];
}

void generic_write(void* obj, uint16_t addr, uint8_t val) {
  Memory* mem = obj;
  assert(addr < mem->size);
  mem->bytes[addr] = val;
}

void generic_reset(void* obj) {
  Memory* mem = obj;
  memset(mem->bytes, 0, mem->size);
}
