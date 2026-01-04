#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memory.h"

#define CPU_REGISTERS 8

typedef struct {
    uint8_t R[CPU_REGISTERS];
    uint16_t pc;
    uint16_t sp;
    uint8_t flags;
} CPU;

void cpu_reset(CPU* cpu);
void cpu_print(const CPU* cpu);
bool cpu_step(CPU* cpu, Memory* memory);
bool cpu_exec(CPU* cpu, Memory* memory, uint64_t max_steps);

#define CPU_FLAG_Z 0
#define CPU_FLAG_C 1
#define CPU_SET_FLAG(cpu, flag, cond)                                                              \
    do {                                                                                           \
        if (cond) {                                                                                \
            cpu->flags |= (1u << flag);                                                            \
        } else {                                                                                   \
            cpu->flags &= ~(1u << flag);                                                           \
        }                                                                                          \
    } while (0)

typedef enum {
    OPCODE_UNKNOWN = 0,
#define X(name, value) OPCODE_##name = value,
#include "opcodes.def"
#undef X
} OpCode;

typedef struct {
    OpCode opcode;
    const char* name;
} OpCodeInfo;

static const OpCodeInfo opcode_table[] = {
#define X(name, value) {OPCODE_##name, #name},
#include "opcodes.def"
#undef X
};

static const size_t OPCODE_COUNT = sizeof opcode_table / sizeof opcode_table[0];

const char* mnemonic_from_opcode(OpCode opcode);
OpCode opcode_from_mnemonic(const char* mnemonic);
