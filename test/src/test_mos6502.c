#include "b6502/memory.h"
#include "b6502/mos6502.h"
#include "unity.h"
#include "unity_fixture.h"

#define MEM_SIZE 65536

static Mos6502* cpu = NULL;
static Memory* mem = NULL;

TEST_GROUP(MOS6502);

TEST_SETUP(MOS6502) {
  cpu = mos6502_create();
  mem = memory_generic_create(MEM_SIZE);
  map_handler(cpu->bus, mem, 0, 0xFFFF);
}

TEST_TEAR_DOWN(MOS6502) {
  rc_strong_release((void*)&cpu);
  rc_strong_release((void*)&mem);
}

TEST(MOS6502, klaus_test) {
  if (read_rom("test/resources/6502_functional_test.bin", mem->bytes, sizeof(*mem->bytes), 0x10000)
      == -1) {
    TEST_ASSERT(false);
  }

  cpu->pc = 0x400;
  while (cpu->pc != 0x3469) {
    uint16_t opc = cpu->pc;
    step(cpu);
    if (cpu->pc == opc) {
      LOG_ERROR("Error at PC: 0x%04X\n", cpu->pc);
      TEST_ASSERT(false);
    }
  }

  LOG_DEBUG("Functional test passed!\n");
  TEST_ASSERT(true);
}

TEST_GROUP_RUNNER(MOS6502) { RUN_TEST_CASE(MOS6502, klaus_test) }
