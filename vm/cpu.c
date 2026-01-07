#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "binary.h"
#include "cpu.h"
#include "memory.h"

bool tiny16_cpu_tracing = false;

void tiny16_cpu_reset(Tiny16CPU* cpu) {
    cpu->pc = TINY16_MEMORY_CODE_BEGIN;
    cpu->sp = TINY16_MEMORY_STACK_END;
    memset(cpu->R, 0, TINY16_CPU_REGISTERS);
    cpu->flags = 0;
}

void tiny16_cpu_print(const Tiny16CPU* cpu) {
    printf("CPU\n\n");

    for (int i = 0; i < TINY16_CPU_REGISTERS; ++i) {
        printf("  --- R%d ---", i);
    }
    printf("\n");

    for (int i = 0; i < TINY16_CPU_REGISTERS; ++i) {
        printf("  0b" TINY16_BIN8_FMT, TINY16_BIN8(cpu->R[i]));
    }
    printf("\n");

    for (int i = 0; i < TINY16_CPU_REGISTERS; ++i) {
        printf("  0x%08X", cpu->R[i]);
    }
    printf("\n");

    for (int i = 0; i < TINY16_CPU_REGISTERS; ++i) {
        printf("    %08d", cpu->R[i]);
    }
    printf("\n\n");

    printf("  ------- PC -------  ------- SP -------  -- Flag  CZ\n");
    printf("  0b" TINY16_BIN16_FMT "  0b" TINY16_BIN16_FMT "  0b " TINY16_BIN8_FMT "\n",
           TINY16_BIN16(cpu->pc), TINY16_BIN16(cpu->sp), TINY16_BIN8(cpu->flags));
    printf("  0x%016X  0x%016X\n", cpu->pc, cpu->sp);
    printf("    %016d    %016d\n", cpu->pc, cpu->sp);
}

inline static uint16_t tiny16_cpu_addr16(const Tiny16CPU* cpu) {
    return ((uint16_t)cpu->R[6] << 8) | cpu->R[7];
}

bool tiny16_cpu_step(Tiny16CPU* cpu, Tiny16Memory* memory) {
    if (cpu->pc < TINY16_MEMORY_CODE_BEGIN || cpu->pc + 2 >= TINY16_MEMORY_CODE_END) {
        fprintf(stderr, "[CRITICAL] PC left code segment: 0x%04X (0x%04X-0x%04X)\n", cpu->pc,
                TINY16_MEMORY_CODE_BEGIN, TINY16_MEMORY_CODE_END);
        return false;
    }

    Tiny16OpCode opcode = memory->bytes[cpu->pc++];
    uint8_t arg1 = memory->bytes[cpu->pc++];
    uint8_t arg2 = memory->bytes[cpu->pc++];

    switch (opcode) {
    case TINY16_OPCODE_LOADI: {
        cpu->R[arg1] = arg2;
    }; break;

    case TINY16_OPCODE_LOAD: {
        uint16_t addr = tiny16_cpu_addr16(cpu);
        cpu->R[arg1] = memory->bytes[addr];
    }; break;

    case TINY16_OPCODE_STORE: {
        uint16_t addr = tiny16_cpu_addr16(cpu);
        if (addr >= TINY16_MEMORY_CODE_BEGIN && addr < TINY16_MEMORY_CODE_END) {
            fprintf(stderr, "[CRITICAL] STORE into code segment: 0x%04X (R6=0x%02X, R7=0x%02X)\n",
                    addr, cpu->R[6], cpu->R[7]);
            return false;
        }
        memory->bytes[addr] = cpu->R[arg1];
    }; break;

    case TINY16_OPCODE_MOV: {
        cpu->R[arg1] = cpu->R[arg2];
    }; break;

    case TINY16_OPCODE_ADD: {
        uint16_t a = cpu->R[arg1];
        uint16_t b = cpu->R[arg2];
        uint16_t res = a + b;
        cpu->R[arg1] = res & UINT8_MAX;
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, res > 0xFF);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
    }; break;

    case TINY16_OPCODE_SUB: {
        uint16_t a = cpu->R[arg1];
        uint16_t b = cpu->R[arg2];
        uint16_t res = a - b;
        cpu->R[arg1] = res & UINT8_MAX;
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, a < b);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
    }; break;

    case TINY16_OPCODE_INC: {
        cpu->R[arg1] = (cpu->R[arg1] + 1) & 0xFF;
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, 0);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
    }; break;

    case TINY16_OPCODE_DEC: {
        cpu->R[arg1] = (cpu->R[arg1] - 1) & 0xFF;
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, 0);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
    }; break;

    case TINY16_OPCODE_AND: {
        cpu->R[arg1] = cpu->R[arg1] & cpu->R[arg2];
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, 0);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
    }; break;

    case TINY16_OPCODE_OR: {
        cpu->R[arg1] = cpu->R[arg1] | cpu->R[arg2];
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, 0);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
    }; break;

    case TINY16_OPCODE_XOR: {
        cpu->R[arg1] = cpu->R[arg1] ^ cpu->R[arg2];
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, 0);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
    }; break;

    case TINY16_OPCODE_SHL: {
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, (cpu->R[arg1] & 0x80) >> 7);
        cpu->R[arg1] <<= 1u;
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
    }; break;

    case TINY16_OPCODE_SHR: {
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, cpu->R[arg1] & 0x01);
        cpu->R[arg1] >>= 1u;
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
    }; break;

    case TINY16_OPCODE_PUSH: {
        uint16_t addr = cpu->sp - 1;
        if (addr < TINY16_MEMORY_STACK_BEGIN) {
            fprintf(stderr, "[CRITICAL] Stack overflow: 0x%04X (valid range is 0x%02X-0x%02X)\n",
                    addr, TINY16_MEMORY_STACK_BEGIN, TINY16_MEMORY_STACK_END);
            return false;
        }
        memory->bytes[cpu->sp--] = cpu->R[arg1];
    }; break;

    case TINY16_OPCODE_POP: {
        uint16_t addr = cpu->sp + 1;
        if (addr > TINY16_MEMORY_STACK_END) {
            fprintf(stderr, "[CRITICAL] Stack underflow: 0x%04X (valid range is 0x%02X-0x%02X)\n",
                    addr, TINY16_MEMORY_STACK_BEGIN, TINY16_MEMORY_STACK_END);
            return false;
        }
        cpu->R[arg1] = memory->bytes[++cpu->sp];
    }; break;

    case TINY16_OPCODE_JMP: {
        uint16_t addr = ((uint16_t)arg1 << 8) | arg2;
        cpu->pc = addr;
    }; break;

    case TINY16_OPCODE_JZ: {
        uint16_t addr = ((uint16_t)arg1 << 8) | arg2;
        if (cpu->flags & (1u << TINY16_CPU_FLAG_Z)) {
            cpu->pc = addr;
        }
    }; break;

    case TINY16_OPCODE_JNZ: {
        uint16_t addr = ((uint16_t)arg1 << 8) | arg2;
        if (!(cpu->flags & (1u << TINY16_CPU_FLAG_Z))) {
            cpu->pc = addr;
        }
    }; break;

    case TINY16_OPCODE_HALT: {
        return false;
    }; break;

    default: {
        fprintf(stderr, "[CRITICAL] Opcode not found: %X\n", opcode);
        return false;
    }
    }
    return true;
}

bool tiny16_cpu_exec(Tiny16CPU* cpu, Tiny16Memory* memory, uint64_t max_steps) {
    uint64_t step = 0;
    bool success;
    const int max_ops = 40;
    char ops[max_ops];

    do {
        if (tiny16_cpu_tracing) {
            uint16_t opcode = memory->bytes[cpu->pc];
            uint16_t arg1 = memory->bytes[cpu->pc + 1];
            uint16_t arg2 = memory->bytes[cpu->pc + 2];

            printf("  PC [0x%04X] | %02X %02X %02X | %-6s", cpu->pc, opcode, arg1, arg2,
                   tiny16_mnemonic_from_opcode(opcode));

            switch (opcode) {
            case TINY16_OPCODE_LOADI: snprintf(ops, max_ops, "R%d, 0x%02X", arg1, arg2); break;
            case TINY16_OPCODE_LOAD:
                snprintf(ops, max_ops, "R%d\t;; R%d = MEM[R6:R7]", arg1, arg1);
                break;
            case TINY16_OPCODE_STORE:
                snprintf(ops, max_ops, "R%d\t;; MEM[R6:R7] = R%d", arg1, arg1);
                break;
            case TINY16_OPCODE_INC:
            case TINY16_OPCODE_DEC:
            case TINY16_OPCODE_PUSH:
            case TINY16_OPCODE_POP: snprintf(ops, max_ops, "R%d", arg1); break;
            case TINY16_OPCODE_JMP:
            case TINY16_OPCODE_JZ:
            case TINY16_OPCODE_JNZ: snprintf(ops, max_ops, "0x%04X", ((arg1 << 8) | arg2)); break;
            case TINY16_OPCODE_HALT: break;
            default: snprintf(ops, max_ops, "R%d, R%d", arg1, arg2);
            }
        }

        memory->bytes[TINY16_MMIO_TICK_LOW] = step & 0xFF;
        memory->bytes[TINY16_MMIO_TICK_HIGH] = (step >> 8) & 0xFF;

        success = tiny16_cpu_step(cpu, memory);

        if (tiny16_cpu_tracing) {
            printf("%-24s\t | C=%d Z=%d |", ops, TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_C),
                   TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_Z));
            for (uint8_t i = 0; i < TINY16_CPU_REGISTERS; ++i) {
                printf(" R%d=%02X", i, cpu->R[i]);
            }
            printf("\n");
        }
    } while ((++step < max_steps) && success);

    return step <= max_steps;
}

const char* tiny16_mnemonic_from_opcode(Tiny16OpCode opcode) {
    for (size_t i = 0; i < TINY16_OPCODE_COUNT; ++i) {
        if (opcode == opcode_table[i].opcode) {
            return opcode_table[i].name;
        }
    }
    return "UNKNOWN";
}

Tiny16OpCode tiny16_opcode_from_mnemonic(const char* mnemonic) {
    for (size_t i = 0; i < TINY16_OPCODE_COUNT; ++i) {
        if (strcmp(mnemonic, opcode_table[i].name) == 0) {
            return opcode_table[i].opcode;
        }
    }
    return TINY16_OPCODE_UNKNOWN;
}
