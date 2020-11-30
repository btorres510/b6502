#pragma once

/// @file mos6502.h

#include "b6502/base.h"
#include "b6502/bus.h"
#include "b6502/rc.h"
#include "b6502/reset_manager.h"

/**
 * @brief The location of the NMI vector.
 */
#define NMI_VECTOR (uint16_t)(0xFFFA)

/**
 * @brief The location of the reset vector.
 */
#define RES_VECTOR (uint16_t)(0xFFFC)

/**
 * @brief The location of the IRQ vector.
 */
#define IRQ_VECTOR (uint16_t)(0xFFFE)

/**
 * @brief The number of possible 6502 opcodes (including undocumented).
 */
#define NUM_OF_OPCODES (size_t)(256)

/**
 * @brief The addressing modes of a MOS6502 opcode.
 */
typedef enum AddressingMode {
  kAbs,
  kAbsX,
  kAbsY,
  kAcc,
  kImm,
  kImpl,
  kIdxInd,
  kInd,
  kIndIdx,
  kRel,
  kZeroP,
  kZeroPX,
  kZeroPY,
} AddressingMode;

/**
 * @brief The interrupt status of a 6502.
 */
typedef enum Interrupt {
  kNone,
  kIRQ,
  kNMI,
} Interrupt;

/**
 * @brief Representation of the status register bit flags.
 */
typedef enum Flags {
  C = 1 << 0,
  Z = 1 << 1,
  I = 1 << 2,
  D = 1 << 3,
  B = 1 << 4,
  U = 1 << 5,
  V = 1 << 6,
  N = 1 << 7,
} Flags;

/**
 * @brief The MOS6502 struct.
 */
typedef struct Mos6502 {
  // Communication bus
  Bus* bus;

  // Registers
  uint16_t pc;
  uint8_t sp;
  uint8_t a;
  uint8_t x;
  uint8_t y;
  uint8_t sr;

  // Helpers
  uint32_t cycles;
  uint16_t addr;
  uint8_t data;
  AddressingMode current_mode;

  // Interrupt status
  Interrupt intr_status;

} Mos6502;

/**
 * @brief Constructor for a MOS6502 object.
 */
Mos6502* mos6502_create(ResetManager* rm);

/**
 * @brief Raise an IRQ
 *
 * @see IRQ_VECTOR
 * @see Interrupt
 */
void raise_irq(Mos6502* cpu);

/**
 * @brief Raise an NMI
 *
 * @see NMI_VECTOR
 * @see Interrupt
 */
void raise_nmi(Mos6502* cpu);

/**
 * @brief Execute one CPU instruction.
 * @param cpu The MOS6502 object.
 */
void step(Mos6502* cpu);
