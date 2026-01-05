#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memory.h"

#define TINY16_VERSION_MAJOR 0
#define TINY16_VERSION_MINOR 1

#define TINY16_TINY16_CPU_REGISTERS 8

typedef struct {
    uint8_t R[TINY16_TINY16_CPU_REGISTERS];
    uint16_t pc;
    uint16_t sp;
    uint8_t flags;
} Tiny16CPU;

void tiny16_cpu_reset(Tiny16CPU* cpu);
void tiny16_cpu_print(const Tiny16CPU* cpu);
bool tiny16_cpu_step(Tiny16CPU* cpu, Tiny16Memory* memory);
bool tiny16_cpu_exec(Tiny16CPU* cpu, Tiny16Memory* memory, uint64_t max_steps);

#define TINY16_CPU_FLAG_Z 0
#define TINY16_CPU_FLAG_C 1
#define TINY16_CPU_FLAG(cpu, f) ((cpu)->flags & (1u << (f)))
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
