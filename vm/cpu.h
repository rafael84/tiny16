#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memory.h"

#define TINY16_VERSION_MAJOR 0
#define TINY16_VERSION_MINOR 1

#define TINY16_CPU_REGISTERS 8

typedef struct {
    uint8_t R[TINY16_CPU_REGISTERS];
    uint16_t pc;
    uint16_t sp;
    uint8_t flags;
} Tiny16CPU;

extern bool tiny16_cpu_tracing;
void tiny16_cpu_reset(Tiny16CPU* cpu);
void tiny16_cpu_print(const Tiny16CPU* cpu);

bool tiny16_cpu_step(Tiny16CPU* cpu, void* memory_context, tiny16_mem_read_fn memory_read,
                     tiny16_mem_write_fn memory_write);

typedef enum {
    TINY16_ADDR_MODE_BASE = 0x00,   // [PAIR]
    TINY16_ADDR_MODE_INC = 0x01,    // [PAIR]+  (post-increment)
    TINY16_ADDR_MODE_DEC = 0x02,    // [PAIR]-  (post-decrement)
    TINY16_ADDR_MODE_OFFSET = 0x03, // [PAIR + imm8]
} Tiny16AddrMode;

typedef enum {
    TINY16_ADDR_PAIR_01 = 0x00, // R0:R1
    TINY16_ADDR_PAIR_23 = 0x01, // R2:R3
    TINY16_ADDR_PAIR_45 = 0x02, // R4:R5
    TINY16_ADDR_PAIR_67 = 0x03, // R6:R7
} Tiny16AddrPair;

typedef struct {
    // Byte 1: REG(7-5) | MODE(4-3) | PAIR(2-1) | 0
    uint8_t reg;
    Tiny16AddrMode mode;
    Tiny16AddrPair pair;
    // Byte 2: (TINY16_ADDR_MODE_OFFSET)
    uint8_t offset;
} Tiny16Addr;

// REG(7-5) | MODE(4-3) | PAIR(2-1) | 0
#define TINY16_ADDR_BYTE1(reg, mode, pair) (((reg) << 5) | ((mode) << 3) | ((pair) << 1))

#define TINY16_CPU_FLAG_Z 0
#define TINY16_CPU_FLAG_C 1

#define TINY16_CPU_FLAG(flags, f) ((flags >> (f)) & 1u)

#define TINY16_CPU_SET_FLAG(cpu, flag, cond)                                                       \
    do {                                                                                           \
        if (cond) {                                                                                \
            cpu->flags |= (1u << flag);                                                            \
        } else {                                                                                   \
            cpu->flags &= ~(1u << flag);                                                           \
        }                                                                                          \
    } while (0)

typedef enum {
    TINY16_OPCODE_UNKNOWN = 0,
#define X(name, value) TINY16_OPCODE_##name = value,
#include "opcodes.def"
#undef X
} Tiny16OpCode;

typedef struct {
    Tiny16OpCode opcode;
    const char* name;
} Tiny16OpCodeInfo;

static const Tiny16OpCodeInfo opcode_table[] = {
#define X(name, value) {TINY16_OPCODE_##name, #name},
#include "opcodes.def"
#undef X
};

static const size_t TINY16_OPCODE_COUNT = sizeof opcode_table / sizeof opcode_table[0];

const char* tiny16_mnemonic_from_opcode(Tiny16OpCode opcode);
Tiny16OpCode tiny16_opcode_from_mnemonic(const char* mnemonic);

void tiny16_cpu_trace(uint16_t addr, Tiny16OpCode opcode, uint8_t arg1, uint8_t arg2);
