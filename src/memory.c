#include "b6502/memory.h"

#include <assert.h>
#include <stdlib.h>

#include "b6502/rc.h"

static void deinit(void* obj) {
  Memory* mem = obj;
  rc_strong_release((void*)&mem->bytes);
}

Memory* memory_create(size_t size, reset_handler reset, read_handler read, write_handler write) {
  Memory* mem = rc_alloc(sizeof(*mem), deinit);
  mem->bytes = calloc(size, sizeof(*mem->bytes));
  mem->size = size;
  mem->_reset = reset;
  mem->_read = read;
  if (write) {
    mem->_read_only = false;
    mem->_write = write;
  }

  return mem;
}

Memory* memory_generic_create(size_t size) {
  Memory* mem = rc_alloc(sizeof(*mem), deinit);
  mem->bytes = rc_alloc(size * sizeof(*mem->bytes), NULL);
  mem->size = size;
  mem->_reset = generic_reset;
  mem->_read = generic_read;
  mem->_read_only = false;
  mem->_write = generic_write;

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
