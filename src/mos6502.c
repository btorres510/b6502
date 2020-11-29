#include "b6502/mos6502.h"

#include <stdbool.h>

/// Be warned, many explicit casts. GCC's -Wconversion is a picky son of a bitch.

#define STACK(sp) (uint16_t)((sp) | 0x0100)

/////////////////////////////////////////////////
///     Helper functions
/////////////////////////////////////////////////

static inline bool cross(uint16_t x, uint16_t y) { return (uint8_t)(x >> 8) != (uint8_t)(y >> 8); }

static inline uint16_t read16(Bus* bus, uint16_t addr) {
  return (uint16_t)((read(bus, (uint16_t)(addr + 1)) << 8) | read(bus, addr));
}

static inline uint16_t buggy_read16(Bus* bus, uint16_t addr) {
  return (uint8_t)(addr & 0x00FF) == 0x00FF
             ? (uint16_t)(read(bus, addr & 0xFF00) << 8 | read(bus, addr))
             : read16(bus, addr);
}

static inline void set_flag(Mos6502* cpu, Flags f, bool v) {
  if (v) {
    cpu->sr = (uint8_t)(cpu->sr | f);
  } else {
    cpu->sr = (uint8_t)(cpu->sr & ~f);
  }
}

static inline bool get_flag(Mos6502* cpu, Flags f) { return (bool)(cpu->sr & f); }

static inline uint8_t pop8(Mos6502* cpu) { return read(cpu->bus, STACK(++cpu->sp)); }

static inline uint16_t pop16(Mos6502* cpu) {
  cpu->sp = (uint8_t)(cpu->sp + 1);
  return read16(cpu->bus, STACK(cpu->sp++));
}

static inline void push8(Mos6502* cpu, uint8_t val) { write(cpu->bus, STACK(cpu->sp--), val); }

static inline void push16(Mos6502* cpu, uint16_t val) {
  write(cpu->bus, STACK(cpu->sp--), (uint8_t)(val >> 8));
  write(cpu->bus, STACK(cpu->sp--), (uint8_t)val);
}

static inline void zn(Mos6502* cpu, uint8_t val) {
  set_flag(cpu, Z, val == 0x00);
  set_flag(cpu, N, (val & 0x80) == 0x80);
}

static void branch(Mos6502* cpu, bool condition) {
  if (condition) {
    cpu->cycles += 1;
    int8_t offset = (int8_t)(read(cpu->bus, (uint16_t)(cpu->pc - 1)));
    uint16_t new_pc = (uint16_t)(cpu->pc + offset);
    if (cross(cpu->pc, new_pc)) {
      cpu->cycles += 1;
    }

    cpu->pc = new_pc;
  }
}

static inline void write_data(Mos6502* cpu) {
  if (cpu->current_mode == kAcc) {
    cpu->a = cpu->data;
  } else {
    write(cpu->bus, cpu->addr, cpu->data);
  }
}

static void interrupt(Mos6502* cpu, uint16_t vector) {
  cpu->cycles += 7;
  push16(cpu, cpu->pc);
  if (read(cpu->bus, (uint16_t)(cpu->pc - 2)) == 0x00) {  // if BRK
    push8(cpu, (uint8_t)(cpu->sr | 0x30));
  } else {
    push8(cpu, cpu->sr);
  }

  cpu->pc = read16(cpu->bus, vector);
  set_flag(cpu, I, true);
}

/////////////////////////////////////////////////
///     Addressing Modes
/////////////////////////////////////////////////

static int absolute(Mos6502* cpu) {
  cpu->addr = read16(cpu->bus, (uint16_t)(cpu->pc + 1));
  cpu->data = read(cpu->bus, cpu->addr);
  return 0;
}

static int absx(Mos6502* cpu) {
  uint16_t orig_addr = read16(cpu->bus, (uint16_t)(cpu->pc + 1));
  cpu->addr = (uint16_t)(cpu->x + orig_addr);
  cpu->data = read(cpu->bus, cpu->addr);
  if (cross(orig_addr, cpu->addr)) {
    return 1;
  }
  return 0;
}

static int absy(Mos6502* cpu) {
  uint16_t orig_addr = read16(cpu->bus, (uint16_t)(cpu->pc + 1));
  cpu->addr = (uint16_t)(cpu->y + orig_addr);
  cpu->data = read(cpu->bus, cpu->addr);
  if (cross(orig_addr, cpu->addr)) {
    return 1;
  }
  return 0;
}

static int acc(Mos6502* cpu) {
  cpu->data = cpu->a;
  return 0;
}

static int imm(Mos6502* cpu) {
  cpu->data = read(cpu->bus, (uint16_t)(cpu->pc + 1));
  return 0;
}

static int impl(Mos6502* UNUSED(cpu)) { return 0; }

static int indidx(Mos6502* cpu) {
  uint8_t ptr = read(cpu->bus, (uint16_t)(cpu->pc + 1));
  uint16_t ptr2 = read16(cpu->bus, ptr);
  cpu->addr = (uint16_t)(cpu->y + ptr2);
  cpu->data = read(cpu->bus, cpu->addr);
  if (cross(cpu->addr, ptr2)) {
    return 1;
  }
  return 0;
}

static int ind(Mos6502* cpu) {
  cpu->addr = buggy_read16(cpu->bus, read16(cpu->bus, (uint16_t)(cpu->pc + 1)));
  return 0;
}

static int idxind(Mos6502* cpu) {
  uint8_t ptr = cpu->x + read(cpu->bus, (uint16_t)(cpu->pc + 1));
  cpu->addr = read16(cpu->bus, (uint16_t)ptr);
  cpu->data = read(cpu->bus, cpu->addr);
  return 0;
}

static int rel(Mos6502* UNUSED(cpu)) { return 0; }

static int zerop(Mos6502* cpu) {
  cpu->addr = read(cpu->bus, cpu->pc + 1);
  cpu->data = read(cpu->bus, cpu->addr);
  return 0;
}

static int zeropx(Mos6502* cpu) {
  uint8_t addr = (uint8_t)(cpu->x + read(cpu->bus, (uint16_t)(cpu->pc + 1)));
  cpu->addr = addr;
  cpu->data = read(cpu->bus, cpu->addr);
  return 0;
}

static int zeropy(Mos6502* cpu) {
  uint8_t addr = (uint8_t)(cpu->y + read(cpu->bus, (uint16_t)(cpu->pc + 1)));
  cpu->addr = addr;
  cpu->data = read(cpu->bus, cpu->addr);
  return 0;
}

/////////////////////////////////////////////////
///     Opcodes
/////////////////////////////////////////////////

static int op_adc(Mos6502* cpu) {
  uint16_t result = (uint16_t)(cpu->a + cpu->data + get_flag(cpu, C));
  set_flag(cpu, Z, (uint8_t)(result) == 0);
  if (get_flag(cpu, D)) {
    if (((cpu->a & 0xF) + (cpu->data & 0xF) + (get_flag(cpu, C))) > 9) {
      result = (uint16_t)(result + 6);
    }

    set_flag(cpu, N, (uint8_t)(result & 0x80) == 0x80);
    set_flag(cpu, V, !((cpu->a ^ cpu->data) & 0x80) && ((cpu->a ^ result) & 0x80));
    if (result > 0x99) {
      result = (uint16_t)(result + 96);
    }

    set_flag(cpu, C, result > 0x99);

  } else {
    set_flag(cpu, N, (uint8_t)(result & 0x80) == 0x80);
    set_flag(cpu, C, result > 0xFF);
    set_flag(cpu, V, !((cpu->a ^ cpu->data) & 0x80) && ((cpu->a ^ result) & 0x80));
  }

  cpu->a = (uint8_t)(result);

  return 1;
}

static int op_and(Mos6502* cpu) {
  zn(cpu, cpu->a &= cpu->data);
  return 1;
}

static int op_asl(Mos6502* cpu) {
  set_flag(cpu, C, (cpu->data >> 7U) == 0x01);
  zn(cpu, cpu->data = (uint8_t)(cpu->data << 1));
  write_data(cpu);
  return 0;
}

static int op_bcc(Mos6502* cpu) {
  branch(cpu, !get_flag(cpu, C));
  return 0;
}

static int op_bcs(Mos6502* cpu) {
  branch(cpu, get_flag(cpu, C));
  return 0;
}

static int op_beq(Mos6502* cpu) {
  branch(cpu, get_flag(cpu, Z));
  return 0;
}

static int op_bit(Mos6502* cpu) {
  set_flag(cpu, Z, (cpu->a & cpu->data) == 0);
  set_flag(cpu, N, (cpu->data & 0x80) == 0x80);
  set_flag(cpu, V, ((cpu->data & 0x40) >> 0x6) == 0x1);
  return 0;
}

static int op_bmi(Mos6502* cpu) {
  branch(cpu, get_flag(cpu, N));
  return 0;
}

static int op_bne(Mos6502* cpu) {
  branch(cpu, !get_flag(cpu, Z));
  return 0;
}

static int op_bpl(Mos6502* cpu) {
  branch(cpu, !get_flag(cpu, N));
  return 0;
}

static int op_brk(Mos6502* cpu) {
  cpu->pc = (uint16_t)(cpu->pc + 1);
  interrupt(cpu, IRQ_VECTOR);
  return 0;
}

static int op_bvc(Mos6502* cpu) {
  branch(cpu, !get_flag(cpu, V));
  return 0;
}

static int op_bvs(Mos6502* cpu) {
  branch(cpu, get_flag(cpu, V));
  return 0;
}

static int op_clc(Mos6502* cpu) {
  set_flag(cpu, C, false);
  return 0;
}

static int op_cld(Mos6502* cpu) {
  set_flag(cpu, D, false);
  return 0;
}

static int op_cli(Mos6502* cpu) {
  set_flag(cpu, I, false);
  return 0;
}

static int op_clv(Mos6502* cpu) {
  set_flag(cpu, V, false);
  return 0;
}

static int op_cmp(Mos6502* cpu) {
  set_flag(cpu, C, cpu->a >= cpu->data);
  zn(cpu, (uint8_t)(cpu->a - cpu->data));
  return 1;
}

static int op_cpx(Mos6502* cpu) {
  set_flag(cpu, C, cpu->x >= cpu->data);
  zn(cpu, (uint8_t)(cpu->x - cpu->data));
  return 0;
}

static int op_cpy(Mos6502* cpu) {
  set_flag(cpu, C, cpu->y >= cpu->data);
  zn(cpu, (uint8_t)(cpu->y - cpu->data));
  return 0;
}

static int op_dec(Mos6502* cpu) {
  zn(cpu, cpu->data = (uint8_t)(cpu->data - 1));
  write_data(cpu);
  return 0;
}

static int op_dex(Mos6502* cpu) {
  zn(cpu, cpu->x = (uint8_t)(cpu->x - 1));
  return 0;
}

static int op_dey(Mos6502* cpu) {
  zn(cpu, cpu->y = (uint8_t)(cpu->y - 1));
  return 0;
}

static int op_eor(Mos6502* cpu) {
  zn(cpu, cpu->a = (uint8_t)(cpu->a ^ cpu->data));
  return 1;
}

static int op_inc(Mos6502* cpu) {
  zn(cpu, cpu->data = (uint8_t)(cpu->data + 1));
  write_data(cpu);
  return 0;
}

static int op_inx(Mos6502* cpu) {
  zn(cpu, cpu->x = (uint8_t)(cpu->x + 1));
  return 0;
}

static int op_iny(Mos6502* cpu) {
  zn(cpu, cpu->y = (uint8_t)(cpu->y + 1));
  return 0;
}

static int op_jmp(Mos6502* cpu) {
  cpu->pc = cpu->addr;
  return 0;
}

static int op_jsr(Mos6502* cpu) {
  push16(cpu, (uint16_t)(cpu->pc - 1));
  cpu->pc = cpu->addr;
  return 0;
}

static int op_lda(Mos6502* cpu) {
  zn(cpu, cpu->a = cpu->data);
  return 1;
}

static int op_ldx(Mos6502* cpu) {
  zn(cpu, cpu->x = cpu->data);
  return 1;
}

static int op_ldy(Mos6502* cpu) {
  zn(cpu, cpu->y = cpu->data);
  return 1;
}

static int op_lsr(Mos6502* cpu) {
  set_flag(cpu, C, (cpu->data & 0x01) == 0x01);

  zn(cpu, cpu->data = (uint8_t)(cpu->data >> 1));
  write_data(cpu);
  return 0;
}

static int op_nop(Mos6502* UNUSED(cpu)) { return 1; }

static int op_ora(Mos6502* cpu) {
  zn(cpu, cpu->a = (uint8_t)(cpu->a | cpu->data));
  return 1;
}

static int op_pha(Mos6502* cpu) {
  push8(cpu, cpu->a);
  return 0;
}

static int op_php(Mos6502* cpu) {
  push8(cpu, (uint8_t)(cpu->sr | 0x30));
  return 0;
}

static int op_pla(Mos6502* cpu) {
  zn(cpu, cpu->a = pop8(cpu));
  return 0;
}

static int op_plp(Mos6502* cpu) {
  cpu->sr = (uint8_t)(pop8(cpu) | 0x30);
  return 0;
}

static int op_rol(Mos6502* cpu) {
  bool c = get_flag(cpu, C);
  set_flag(cpu, C, (cpu->data & 0x80) == 0x80);

  zn(cpu, cpu->data = (uint8_t)((cpu->data << 1) | c));
  write_data(cpu);
  return 0;
}

static int op_ror(Mos6502* cpu) {
  bool c = get_flag(cpu, C);
  set_flag(cpu, C, (cpu->data & 0x01) == 0x01);

  zn(cpu, cpu->data = (uint8_t)((cpu->data >> 1) | (c << 7)));
  write_data(cpu);
  return 0;
}

static int op_rti(Mos6502* cpu) {
  cpu->sr = (uint8_t)(pop8(cpu) | 0x30);
  cpu->pc = pop16(cpu);
  return 0;
}

static int op_rts(Mos6502* cpu) {
  cpu->pc = (uint16_t)(pop16(cpu) + 1);
  return 0;
}

static int op_sbc(Mos6502* cpu) {
  uint16_t result = (uint16_t)(cpu->a - cpu->data - !get_flag(cpu, C));
  zn(cpu, (uint8_t)(result));
  set_flag(cpu, V, ((cpu->a ^ result) & 0x80) && ((cpu->a ^ cpu->data) & 0x80));
  if (get_flag(cpu, D)) {
    if (((cpu->a & 0xF) - (!get_flag(cpu, C))) < (cpu->data & 0xF)) {
      result = (uint16_t)(result - 6);
    }

    if (result > 0x99) {
      result = (uint16_t)(result - 0x60);
    }
  }
  set_flag(cpu, C, result < 0x100);
  cpu->a = (uint8_t)(result);
  return 1;
}

static int op_sec(Mos6502* cpu) {
  set_flag(cpu, C, true);
  return 0;
}

static int op_sed(Mos6502* cpu) {
  set_flag(cpu, D, true);
  return 0;
}

static int op_sei(Mos6502* cpu) {
  set_flag(cpu, I, true);
  return 0;
}

static int op_sta(Mos6502* cpu) {
  write(cpu->bus, cpu->addr, cpu->a);
  return 0;
}

static int op_stx(Mos6502* cpu) {
  write(cpu->bus, cpu->addr, cpu->x);
  return 0;
}

static int op_sty(Mos6502* cpu) {
  write(cpu->bus, cpu->addr, cpu->y);
  return 0;
}

static int op_tax(Mos6502* cpu) {
  zn(cpu, cpu->x = cpu->a);
  return 0;
}

static int op_tay(Mos6502* cpu) {
  zn(cpu, cpu->y = cpu->a);
  return 0;
}

static int op_tsx(Mos6502* cpu) {
  zn(cpu, cpu->x = cpu->sp);
  return 0;
}

static int op_txa(Mos6502* cpu) {
  zn(cpu, cpu->a = cpu->x);
  return 0;
}

static int op_txs(Mos6502* cpu) {
  cpu->sp = cpu->x;
  return 0;
}

static int op_tya(Mos6502* cpu) {
  zn(cpu, cpu->a = cpu->y);
  return 0;
}

/////////////////////////////////////////////////
///     Undocumented Opcodes
/////////////////////////////////////////////////

static int op_alr(Mos6502* cpu) {
  (void)(op_and(cpu));
  cpu->data = cpu->a;
  cpu->current_mode = kAcc;
  (void)(op_lsr(cpu));
  return 0;
}

static int op_anc(Mos6502* cpu) {
  (void)(op_and(cpu));
  set_flag(cpu, C, (bool)(cpu->a >> 7));
  return 0;
}

static int op_arr(Mos6502* cpu) {
  (void)(op_and(cpu));
  cpu->a = (uint8_t)(cpu->a >> 1);
  set_flag(cpu, C, (bool)(cpu->a >> 6));
  set_flag(cpu, V, (bool)(cpu->a >> 5));
  return 0;
}

static int op_aso(Mos6502* cpu) {
  (void)(op_asl(cpu));
  (void)(op_and(cpu));
  return 0;
}

static int op_axa(Mos6502* cpu) {
  cpu->data = (uint8_t)((cpu->a & cpu->x) & 0x7);
  write_data(cpu);
  return 0;
}

static int op_axs(Mos6502* cpu) {
  zn(cpu, cpu->x & cpu->a);
  write_data(cpu);
  return 0;
}

static int op_dcm(Mos6502* cpu) {
  cpu->data = (uint8_t)(cpu->data - 1);
  (void)(op_cmp(cpu));
  return 0;
}

static int op_ins(Mos6502* cpu) {
  cpu->data = cpu->data + 1;
  (void)(op_sbc(cpu));
  return 0;
}

// https://www.pagetable.com/?p=39
static int op_kil(Mos6502* cpu) {
  LOG_ERROR("KIL instruction! Please reset the emulator!\n");
  LOG_ERROR("PC: 0x%04X\n", cpu->pc);
  LOG_ERROR("State: A:0x%02X X:0x%02X Y:0x%02X SP:0x%02X SR:0x%02X\n", cpu->a, cpu->x, cpu->y,
            cpu->sp, cpu->sr);
  for (;;) {
    // This instruction locks up the 6502 completely, and needs to be reset in order to continue
    // This will leak memory
  }
}

static int op_las(Mos6502* cpu) {
  zn(cpu, cpu->sp = (uint8_t)(cpu->sp & cpu->data));
  cpu->x = cpu->a = cpu->sp;
  return 1;
}

static int op_lax(Mos6502* cpu) {
  (void)(op_lda(cpu));
  cpu->x = cpu->a;  // Instead of using op_tax() directly,
                    // I just load x with a to avoid an extra call to zn()
  return 1;
}

static int op_lse(Mos6502* cpu) {
  (void)(op_lsr(cpu));
  (void)(op_eor(cpu));
  return 0;
}

static int op_oal(Mos6502* cpu) {
  zn(cpu, cpu->a = (uint8_t)(cpu->a & cpu->data));
  cpu->x = cpu->a;
  return 0;
}

static int op_rla(Mos6502* cpu) {
  (void)(op_rol(cpu));
  (void)(op_and(cpu));
  return 0;
}

static int op_rra(Mos6502* cpu) {
  (void)(op_ror(cpu));
  (void)(op_adc(cpu));
  return 0;
}

static int op_say(Mos6502* cpu) {
  cpu->data = (uint8_t)(cpu->y & ((cpu->addr >> 8) + 1));
  write_data(cpu);
  return 0;
}

static int op_sax(Mos6502* cpu) {
  cpu->x = (uint8_t)(cpu->x & cpu->a);
  uint16_t result = (uint16_t)(cpu->data - cpu->x);
  set_flag(cpu, C, result <= 0xFF);
  zn(cpu, (uint8_t)(result));
  return 0;
}

static int op_tas(Mos6502* cpu) {
  cpu->sp = (uint8_t)(cpu->x & cpu->a);
  cpu->data = (uint8_t)(cpu->sp & ((cpu->addr >> 8) + 1));
  write_data(cpu);
  return 0;
}

// http://visual6502.org/wiki/index.php%3Ftitle%3D6502_Opcode_8B_(XAA,_ANE)
// I am following the behavior of a 6502 chip manufactured in the USA.
static int op_xaa(Mos6502* cpu) {
  cpu->a = (uint8_t)((cpu->a | 0xFF) & cpu->x & cpu->data);
  return 0;
}

static int op_xas(Mos6502* cpu) {
  cpu->data = (uint8_t)(cpu->x & ((cpu->addr >> 8) + 1));
  write_data(cpu);
  return 0;
}

typedef int (*handler)(Mos6502*);

struct Opcode {
  const char* const name;
  AddressingMode mode;
  uint32_t cycles;
  uint8_t length;
  handler mode_handler;
  handler opcode_handler;
};

static const struct Opcode opcodes[NUM_OF_OPCODES] = {
    {"BRK", kImpl, 7, 1, &impl, &op_brk},     {"ORA", kIdxInd, 6, 2, &idxind, &op_ora},
    {"KIL", kImpl, 2, 1, &impl, &op_kil},     {"ASO", kIdxInd, 8, 2, &idxind, &op_aso},
    {"NOP", kZeroP, 3, 2, &zerop, &op_nop},   {"ORA", kZeroP, 3, 2, &zerop, &op_ora},
    {"ASL", kZeroP, 5, 2, &zerop, &op_asl},   {"ASO", kZeroP, 5, 2, &zerop, &op_aso},
    {"PHP", kImpl, 3, 1, &impl, &op_php},     {"ORA", kImm, 2, 2, &imm, &op_ora},
    {"ASL", kAcc, 2, 1, &acc, &op_asl},       {"ANC", kImm, 2, 2, &imm, &op_anc},
    {"NOP", kAbs, 4, 3, &absolute, &op_nop},  {"ORA", kAbs, 4, 3, &absolute, &op_ora},
    {"ASL", kAbs, 6, 3, &absolute, &op_asl},  {"ASO", kAbs, 6, 3, &absolute, &op_aso},
    {"BPL", kRel, 2, 2, &rel, &op_bpl},       {"ORA", kIndIdx, 5, 2, &indidx, &op_ora},
    {"KIL", kImpl, 2, 1, &impl, &op_kil},     {"ASO", kIndIdx, 8, 2, &indidx, &op_aso},
    {"NOP", kZeroPX, 4, 2, &zeropx, &op_nop}, {"ORA", kZeroPX, 4, 2, &zeropx, &op_ora},
    {"ASL", kZeroPX, 6, 2, &zeropx, &op_asl}, {"ASO", kZeroPX, 6, 2, &zeropx, &op_aso},
    {"CLC", kImpl, 2, 1, &impl, &op_clc},     {"ORA", kAbsY, 4, 3, &absy, &op_ora},
    {"NOP", kImpl, 2, 1, &impl, &op_nop},     {"ASO", kAbsY, 7, 3, &absy, &op_aso},
    {"NOP", kAbsX, 4, 3, &absx, &op_nop},     {"ORA", kAbsX, 4, 3, &absx, &op_ora},
    {"ASL", kAbsX, 7, 3, &absx, &op_asl},     {"ASO", kAbsX, 7, 3, &absx, &op_aso},
    {"JSR", kAbs, 6, 3, &absolute, &op_jsr},  {"AND", kIdxInd, 6, 2, &idxind, &op_and},
    {"KIL", kImpl, 2, 1, &impl, &op_kil},     {"RLA", kIdxInd, 8, 2, &idxind, &op_rla},
    {"BIT", kZeroP, 3, 2, &zerop, &op_bit},   {"AND", kZeroP, 3, 2, &zerop, &op_and},
    {"ROL", kZeroP, 5, 2, &zerop, &op_rol},   {"RLA", kZeroP, 5, 2, &zerop, &op_rla},
    {"PLP", kImpl, 4, 1, &impl, &op_plp},     {"AND", kImm, 2, 2, &imm, &op_and},
    {"ROL", kAcc, 2, 1, &acc, &op_rol},       {"ANC", kImm, 2, 2, &imm, &op_anc},
    {"BIT", kAbs, 4, 3, &absolute, &op_bit},  {"AND", kAbs, 4, 3, &absolute, &op_and},
    {"ROL", kAbs, 6, 3, &absolute, &op_rol},  {"RLA", kAbs, 6, 3, &absolute, &op_rla},
    {"BMI", kRel, 2, 2, &rel, &op_bmi},       {"AND", kIndIdx, 5, 2, &indidx, &op_and},
    {"KIL", kImpl, 2, 1, &impl, &op_kil},     {"RLA", kIndIdx, 8, 2, &indidx, &op_rla},
    {"NOP", kZeroPX, 4, 2, &zeropx, &op_nop}, {"AND", kZeroPX, 4, 2, &zeropx, &op_and},
    {"ROL", kZeroPX, 6, 2, &zeropx, &op_rol}, {"RLA", kZeroPX, 6, 2, &zeropx, &op_rla},
    {"SEC", kImpl, 2, 1, &impl, &op_sec},     {"AND", kAbsY, 4, 3, &absy, &op_and},
    {"NOP", kImpl, 2, 1, &impl, &op_nop},     {"RLA", kAbsY, 7, 3, &absy, &op_rla},
    {"NOP", kAbsX, 4, 3, &absx, &op_nop},     {"AND", kAbsX, 4, 3, &absx, &op_and},
    {"ROL", kAbsX, 7, 3, &absx, &op_rol},     {"RLA", kAbsX, 7, 3, &absx, &op_rla},
    {"RTI", kImpl, 6, 1, &impl, &op_rti},     {"EOR", kIdxInd, 6, 2, &idxind, &op_eor},
    {"KIL", kImpl, 2, 1, &impl, &op_kil},     {"LSE", kIdxInd, 8, 2, &idxind, &op_lse},
    {"NOP", kZeroP, 3, 2, &zerop, &op_nop},   {"EOR", kZeroP, 3, 2, &zerop, &op_eor},
    {"LSR", kZeroP, 5, 2, &zerop, &op_lsr},   {"LSE", kZeroP, 5, 2, &zerop, &op_lse},
    {"PHA", kImpl, 3, 1, &impl, &op_pha},     {"EOR", kImm, 2, 2, &imm, &op_eor},
    {"LSR", kAcc, 2, 1, &acc, &op_lsr},       {"ALR", kImm, 2, 2, &imm, &op_alr},
    {"JMP", kAbs, 3, 3, &absolute, &op_jmp},  {"EOR", kAbs, 4, 3, &absolute, &op_eor},
    {"LSR", kAbs, 6, 3, &absolute, &op_lsr},  {"LSE", kAbs, 6, 3, &absolute, &op_lse},
    {"BVC", kRel, 2, 2, &rel, &op_bvc},       {"EOR", kIndIdx, 5, 2, &indidx, &op_eor},
    {"KIL", kImpl, 2, 1, &impl, &op_kil},     {"LSE", kIndIdx, 8, 2, &indidx, &op_lse},
    {"NOP", kZeroPX, 4, 2, &zeropx, &op_nop}, {"EOR", kZeroPX, 4, 2, &zeropx, &op_eor},
    {"LSR", kZeroPX, 6, 2, &zeropx, &op_lsr}, {"LSE", kZeroPX, 6, 2, &zeropx, &op_lse},
    {"CLI", kImpl, 2, 1, &impl, &op_cli},     {"EOR", kAbsY, 4, 3, &absy, &op_eor},
    {"NOP", kImpl, 2, 1, &impl, &op_nop},     {"LSE", kAbsY, 7, 3, &absy, &op_lse},
    {"NOP", kAbsX, 4, 3, &absx, &op_nop},     {"EOR", kAbsX, 4, 3, &absx, &op_eor},
    {"LSR", kAbsX, 7, 3, &absx, &op_lsr},     {"LSE", kAbsX, 7, 3, &absx, &op_lse},
    {"RTS", kImpl, 6, 1, &impl, &op_rts},     {"ADC", kIdxInd, 6, 2, &idxind, &op_adc},
    {"KIL", kImpl, 2, 1, &impl, &op_kil},     {"RRA", kIdxInd, 8, 2, &idxind, &op_rra},
    {"NOP", kZeroP, 3, 2, &zerop, &op_nop},   {"ADC", kZeroP, 3, 2, &zerop, &op_adc},
    {"ROR", kZeroP, 5, 2, &zerop, &op_ror},   {"RRA", kZeroP, 5, 2, &zerop, &op_rra},
    {"PLA", kImpl, 4, 1, &impl, &op_pla},     {"ADC", kImm, 2, 2, &imm, &op_adc},
    {"ROR", kAcc, 2, 1, &acc, &op_ror},       {"ARR", kImm, 2, 2, &imm, &op_arr},
    {"JMP", kInd, 5, 3, &ind, &op_jmp},       {"ADC", kAbs, 4, 3, &absolute, &op_adc},
    {"ROR", kAbs, 6, 3, &absolute, &op_ror},  {"RRA", kAbs, 6, 3, &absolute, &op_rra},
    {"BVS", kRel, 2, 2, &rel, &op_bvs},       {"ADC", kIndIdx, 5, 2, &indidx, &op_adc},
    {"KIL", kImpl, 2, 1, &impl, &op_kil},     {"RRA", kIndIdx, 8, 2, &indidx, &op_rra},
    {"NOP", kZeroPX, 4, 2, &zeropx, &op_nop}, {"ADC", kZeroPX, 4, 2, &zeropx, &op_adc},
    {"ROR", kZeroPX, 6, 2, &zeropx, &op_ror}, {"RRA", kZeroPX, 6, 2, &zeropx, &op_rra},
    {"SEI", kImpl, 2, 1, &impl, &op_sei},     {"ADC", kAbsY, 4, 3, &absy, &op_adc},
    {"NOP", kImpl, 2, 1, &impl, &op_nop},     {"RRA", kAbsY, 7, 3, &absy, &op_rra},
    {"NOP", kAbsX, 4, 3, &absx, &op_nop},     {"ADC", kAbsX, 4, 3, &absx, &op_adc},
    {"ROR", kAbsX, 7, 3, &absx, &op_ror},     {"RRA", kAbsX, 7, 3, &absx, &op_rra},
    {"NOP", kImm, 6, 2, &imm, &op_nop},       {"STA", kIdxInd, 6, 2, &idxind, &op_sta},
    {"NOP", kImm, 2, 2, &imm, &op_nop},       {"AXS", kIdxInd, 6, 2, &idxind, &op_axs},
    {"STY", kZeroP, 3, 2, &zerop, &op_sty},   {"STA", kZeroP, 3, 2, &zerop, &op_sta},
    {"STX", kZeroP, 3, 2, &zerop, &op_stx},   {"AXS", kZeroP, 3, 2, &zerop, &op_axs},
    {"DEY", kImpl, 2, 1, &impl, &op_dey},     {"NOP", kImm, 2, 2, &imm, &op_nop},
    {"TXA", kImpl, 2, 1, &impl, &op_txa},     {"XAA", kImm, 2, 2, &imm, &op_xaa},
    {"STY", kAbs, 4, 3, &absolute, &op_sty},  {"STA", kAbs, 4, 3, &absolute, &op_sta},
    {"STX", kAbs, 4, 3, &absolute, &op_stx},  {"AXS", kAbs, 4, 3, &absolute, &op_axs},
    {"BCC", kRel, 2, 2, &rel, &op_bcc},       {"STA", kIndIdx, 6, 2, &indidx, &op_sta},
    {"KIL", kImpl, 2, 1, &impl, &op_kil},     {"AXA", kIndIdx, 6, 2, &indidx, &op_axa},
    {"STY", kZeroPX, 4, 2, &zeropx, &op_sty}, {"STA", kZeroPX, 4, 2, &zeropx, &op_sta},
    {"STX", kZeroPY, 4, 2, &zeropy, &op_stx}, {"AXS", kZeroPY, 4, 2, &zeropy, &op_axs},
    {"TYA", kImpl, 2, 1, &impl, &op_tya},     {"STA", kAbsY, 5, 3, &absy, &op_sta},
    {"TXS", kImpl, 2, 1, &impl, &op_txs},     {"TAS", kAbsY, 5, 3, &absy, &op_tas},
    {"SAY", kAbsX, 5, 3, &absx, &op_say},     {"STA", kAbsX, 5, 3, &absx, &op_sta},
    {"XAS", kAbsY, 5, 3, &absy, &op_xas},     {"AXA", kAbsY, 5, 3, &absy, &op_axa},
    {"LDY", kImm, 2, 2, &imm, &op_ldy},       {"LDA", kIdxInd, 6, 2, &idxind, &op_lda},
    {"LDX", kImm, 2, 2, &imm, &op_ldx},       {"LAX", kIdxInd, 6, 2, &idxind, &op_lax},
    {"LDY", kZeroP, 3, 2, &zerop, &op_ldy},   {"LDA", kZeroP, 3, 2, &zerop, &op_lda},
    {"LDX", kZeroP, 3, 2, &zerop, &op_ldx},   {"LAX", kZeroP, 3, 2, &zerop, &op_lax},
    {"TAY", kImpl, 2, 1, &impl, &op_tay},     {"LDA", kImm, 2, 2, &imm, &op_lda},
    {"TAX", kImpl, 2, 1, &impl, &op_tax},     {"OAL", kImm, 2, 2, &imm, &op_oal},
    {"LDY", kAbs, 4, 3, &absolute, &op_ldy},  {"LDA", kAbs, 4, 3, &absolute, &op_lda},
    {"LDX", kAbs, 4, 3, &absolute, &op_ldx},  {"LAX", kAbs, 4, 3, &absolute, &op_lax},
    {"BCS", kRel, 2, 2, &rel, &op_bcs},       {"LDA", kIndIdx, 5, 2, &indidx, &op_lda},
    {"KIL", kImpl, 2, 1, &impl, &op_kil},     {"LAX", kIndIdx, 5, 2, &indidx, &op_lax},
    {"LDY", kZeroPX, 4, 2, &zeropx, &op_ldy}, {"LDA", kZeroPX, 4, 2, &zeropx, &op_lda},
    {"LDX", kZeroPY, 4, 2, &zeropy, &op_ldx}, {"LAX", kZeroPY, 4, 2, &zeropy, &op_lax},
    {"CLV", kImpl, 2, 1, &impl, &op_clv},     {"LDA", kAbsY, 4, 3, &absy, &op_lda},
    {"TSX", kImpl, 2, 1, &impl, &op_tsx},     {"LAS", kAbsY, 4, 3, &absy, &op_las},
    {"LDY", kAbsX, 4, 3, &absx, &op_ldy},     {"LDA", kAbsX, 4, 3, &absx, &op_lda},
    {"LDX", kAbsY, 4, 3, &absy, &op_ldx},     {"LAX", kAbsY, 4, 3, &absy, &op_lax},
    {"CPY", kImm, 2, 2, &imm, &op_cpy},       {"CMP", kIdxInd, 6, 2, &idxind, &op_cmp},
    {"NOP", kImm, 2, 2, &imm, &op_nop},       {"DCM", kIdxInd, 8, 2, &idxind, &op_dcm},
    {"CPY", kZeroP, 3, 2, &zerop, &op_cpy},   {"CMP", kZeroP, 3, 2, &zerop, &op_cmp},
    {"DEC", kZeroP, 5, 2, &zerop, &op_dec},   {"DCM", kZeroP, 5, 2, &zerop, &op_dcm},
    {"INY", kImpl, 2, 1, &impl, &op_iny},     {"CMP", kImm, 2, 2, &imm, &op_cmp},
    {"DEX", kImpl, 2, 1, &impl, &op_dex},     {"SAX", kImm, 2, 2, &imm, &op_sax},
    {"CPY", kAbs, 4, 3, &absolute, &op_cpy},  {"CMP", kAbs, 4, 3, &absolute, &op_cmp},
    {"DEC", kAbs, 6, 3, &absolute, &op_dec},  {"DCM", kAbs, 6, 3, &absolute, &op_dcm},
    {"BNE", kRel, 2, 2, &rel, &op_bne},       {"CMP", kIndIdx, 5, 2, &indidx, &op_cmp},
    {"KIL", kImpl, 2, 1, &impl, &op_kil},     {"DCM", kIndIdx, 8, 2, &indidx, &op_dcm},
    {"NOP", kZeroPX, 4, 2, &zeropx, &op_nop}, {"CMP", kZeroPX, 4, 2, &zeropx, &op_cmp},
    {"DEC", kZeroPX, 6, 2, &zeropx, &op_dec}, {"DCM", kZeroPX, 6, 2, &zeropx, &op_dcm},
    {"CLD", kImpl, 2, 1, &impl, &op_cld},     {"CMP", kAbsY, 4, 3, &absy, &op_cmp},
    {"NOP", kImpl, 2, 1, &impl, &op_nop},     {"DCM", kAbsY, 7, 3, &absy, &op_dcm},
    {"NOP", kAbsX, 4, 3, &absx, &op_nop},     {"CMP", kAbsX, 4, 3, &absx, &op_cmp},
    {"DEC", kAbsX, 7, 3, &absx, &op_dec},     {"DCM", kAbsX, 7, 3, &absx, &op_dcm},
    {"CPX", kImm, 2, 2, &imm, &op_cpx},       {"SBC", kIdxInd, 6, 2, &idxind, &op_sbc},
    {"NOP", kImm, 2, 2, &imm, &op_nop},       {"INS", kIdxInd, 8, 2, &idxind, &op_ins},
    {"CPX", kZeroP, 3, 2, &zerop, &op_cpx},   {"SBC", kZeroP, 3, 2, &zerop, &op_sbc},
    {"INC", kZeroP, 5, 2, &zerop, &op_inc},   {"INS", kZeroP, 5, 2, &zerop, &op_ins},
    {"INX", kImpl, 2, 1, &impl, &op_inx},     {"SBC", kImm, 2, 2, &imm, &op_sbc},
    {"NOP", kImpl, 2, 1, &impl, &op_nop},     {"SBC", kImm, 2, 2, &imm, &op_sbc},
    {"CPX", kAbs, 4, 3, &absolute, &op_cpx},  {"SBC", kAbs, 4, 3, &absolute, &op_sbc},
    {"INC", kAbs, 6, 3, &absolute, &op_inc},  {"INS", kAbs, 6, 3, &absolute, &op_ins},
    {"BEQ", kRel, 2, 2, &rel, &op_beq},       {"SBC", kIndIdx, 5, 2, &indidx, &op_sbc},
    {"KIL", kImpl, 2, 1, &impl, &op_kil},     {"INS", kIndIdx, 8, 2, &indidx, &op_ins},
    {"NOP", kZeroPX, 4, 2, &zeropx, &op_nop}, {"SBC", kZeroPX, 4, 2, &zeropx, &op_sbc},
    {"INC", kZeroPX, 6, 2, &zeropx, &op_inc}, {"INS", kZeroPX, 6, 2, &zeropx, &op_ins},
    {"SED", kImpl, 2, 1, &impl, &op_sed},     {"SBC", kAbsY, 4, 3, &absy, &op_sbc},
    {"NOP", kImpl, 2, 1, &impl, &op_nop},     {"INS", kAbsY, 7, 3, &absy, &op_ins},
    {"NOP", kAbsX, 4, 3, &absx, &op_nop},     {"SBC", kAbsX, 4, 3, &absx, &op_sbc},
    {"INC", kAbsX, 7, 3, &absx, &op_inc},     {"INS", kAbsX, 7, 3, &absx, &op_ins},
};

static void deinit(void* obj) {
  Mos6502* cpu = obj;
  for (size_t page = 0; page < NUMBER_OF_PAGES; page++) {
    rc_weak_release((void*)&cpu->bus->handlers[page]);
  }
  rc_strong_release((void*)&cpu->bus);
}

/////////////////////////////////////////////////
///     Public API
/////////////////////////////////////////////////

Mos6502* mos6502_create(void) {
  Mos6502* cpu = rc_alloc(sizeof(*cpu), deinit);
  cpu->bus = rc_alloc(sizeof(*cpu->bus), NULL);
  cpu->sr = 0x34;
  cpu->sp = 0xFD;
  return cpu;
}

void raise_irq(Mos6502* cpu) {
  if (cpu->intr_status != kNMI) {
    cpu->intr_status = kIRQ;
  }
}

void raise_nmi(Mos6502* cpu) { cpu->intr_status = kNMI; }

void step(Mos6502* cpu) {
  switch (cpu->intr_status) {
    case kNone:
      break;
    case kIRQ:
      interrupt(cpu, IRQ_VECTOR);
      break;
    case kNMI:
      interrupt(cpu, NMI_VECTOR);
      break;
  }

  uint8_t opcode = read(cpu->bus, cpu->pc);
  cpu->current_mode = opcodes[opcode].mode;

  int mode_cycles = (*opcodes[opcode].mode_handler)(cpu);
  cpu->cycles += opcodes[opcode].cycles;
  cpu->pc += (uint16_t)(opcodes[opcode].length);
  int opcode_cycles = (*opcodes[opcode].opcode_handler)(cpu);

  cpu->cycles += (uint32_t)(mode_cycles & opcode_cycles);
}
