#include "cpu.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "binary.h"
#include "memory.h"

void cpu_reset(CPU* cpu) {
    cpu->pc = MEMORY_CODE_BEGIN;
    cpu->sp = MEMORY_STACK_BEGIN;
    memset(cpu->R, 0, CPU_REGISTERS);
    cpu->flags = 0;
}

void cpu_print(const CPU* cpu) {
    printf("CPU\n\n");

    for (int i = 0; i < CPU_REGISTERS; ++i) {
        printf("  --- R%d ---", i);
    }
    printf("\n");

    for (int i = 0; i < CPU_REGISTERS; ++i) {
        printf("  0b" BIN8_FMT, BIN8(cpu->R[i]));
    }
    printf("\n");

    for (int i = 0; i < CPU_REGISTERS; ++i) {
        printf("  0x%08X", cpu->R[i]);
    }
    printf("\n");

    for (int i = 0; i < CPU_REGISTERS; ++i) {
        printf("    %08d", cpu->R[i]);
    }
    printf("\n\n");

    printf("  ------- PC -------  ------- SP -------  -- Flag  CZ\n");
    printf("  0b" BIN16_FMT "  0b" BIN16_FMT "  0b " BIN8_FMT "\n", BIN16(cpu->pc), BIN16(cpu->sp),
           BIN8(cpu->flags));
    printf("  0x%016X  0x%016X\n", cpu->pc, cpu->sp);
    printf("    %016d    %016d\n", cpu->pc, cpu->sp);
}

inline static uint16_t cpu_addr16(const CPU* cpu) { return ((uint16_t)cpu->R[6] << 8) | cpu->R[7]; }

bool cpu_step(CPU* cpu, Memory* memory) {
    if (cpu->pc < MEMORY_CODE_BEGIN || cpu->pc + 2 >= MEMORY_CODE_END) {
        fprintf(stderr, "PC left code segment: 0x%04X\n", cpu->pc);
        return false;
    }

    OpCode opcode = memory->bytes[cpu->pc++];
    uint8_t arg1 = memory->bytes[cpu->pc++];
    uint8_t arg2 = memory->bytes[cpu->pc++];

    switch (opcode) {
    case OPCODE_LOADI: {
        cpu->R[arg1] = arg2;
    }; break;

    case OPCODE_LOAD: {
        uint16_t addr = cpu_addr16(cpu);
        cpu->R[arg1] = memory->bytes[addr];
    }; break;

    case OPCODE_STORE: {
        uint16_t addr = cpu_addr16(cpu);
        if (addr >= MEMORY_CODE_BEGIN && addr < MEMORY_CODE_END) {
            fprintf(stderr, "STORE into code segment: 0x%04X\n", addr);
            return false;
        }
        memory->bytes[addr] = cpu->R[arg1];
    }; break;

    case OPCODE_ADD: {
        uint16_t a = cpu->R[arg1];
        uint16_t b = cpu->R[arg2];
        uint16_t res = a + b;
        cpu->R[arg1] = res & UINT8_MAX;
        CPU_SET_FLAG(cpu, CPU_FLAG_C, res > 0xFF);
        CPU_SET_FLAG(cpu, CPU_FLAG_Z, cpu->R[arg1] == 0);
    }; break;

    case OPCODE_SUB: {
        uint16_t a = cpu->R[arg1];
        uint16_t b = cpu->R[arg2];
        uint16_t res = a - b;
        cpu->R[arg1] = res & UINT8_MAX;
        CPU_SET_FLAG(cpu, CPU_FLAG_C, a < b);
        CPU_SET_FLAG(cpu, CPU_FLAG_Z, cpu->R[arg1] == 0);
    }; break;

    case OPCODE_INC: {
        cpu->R[arg1] = (cpu->R[arg1] + 1) & 0xFF;
        CPU_SET_FLAG(cpu, CPU_FLAG_C, 0);
        CPU_SET_FLAG(cpu, CPU_FLAG_Z, cpu->R[arg1] == 0);
    }; break;

    case OPCODE_DEC: {
        cpu->R[arg1] = (cpu->R[arg1] - 1) & 0xFF;
        CPU_SET_FLAG(cpu, CPU_FLAG_C, 0);
        CPU_SET_FLAG(cpu, CPU_FLAG_Z, cpu->R[arg1] == 0);
    }; break;

    case OPCODE_AND: {
        cpu->R[arg1] = cpu->R[arg1] & cpu->R[arg2];
        CPU_SET_FLAG(cpu, CPU_FLAG_C, 0);
        CPU_SET_FLAG(cpu, CPU_FLAG_Z, cpu->R[arg1] == 0);
    }; break;

    case OPCODE_OR: {
        cpu->R[arg1] = cpu->R[arg1] | cpu->R[arg2];
        CPU_SET_FLAG(cpu, CPU_FLAG_C, 0);
        CPU_SET_FLAG(cpu, CPU_FLAG_Z, cpu->R[arg1] == 0);
    }; break;

    case OPCODE_XOR: {
        cpu->R[arg1] = cpu->R[arg1] ^ cpu->R[arg2];
        CPU_SET_FLAG(cpu, CPU_FLAG_C, 0);
        CPU_SET_FLAG(cpu, CPU_FLAG_Z, cpu->R[arg1] == 0);
    }; break;

    case OPCODE_JMP: {
        uint16_t addr = ((uint16_t)arg1 << 8) | arg2;
        cpu->pc = addr;
    }; break;

    case OPCODE_JZ: {
        uint16_t addr = ((uint16_t)arg1 << 8) | arg2;
        if (cpu->flags & (1u << CPU_FLAG_Z)) {
            cpu->pc = addr;
        }
    }; break;

    case OPCODE_JNZ: {
        uint16_t addr = ((uint16_t)arg1 << 8) | arg2;
        if (!(cpu->flags & (1u << CPU_FLAG_Z))) {
            cpu->pc = addr;
        }
    }; break;

    case OPCODE_HALT: {
        return false;
    }; break;

    default: {
        fprintf(stderr, "opcode not found: %X\n", opcode);
        return false;
    }
    }
    return true;
}

bool cpu_exec(CPU* cpu, Memory* memory, uint64_t max_steps) {
    uint64_t step = 0;
    do {
        uint16_t opcode = memory->bytes[cpu->pc];
        uint16_t arg1 = memory->bytes[cpu->pc + 1];
        uint16_t arg2 = memory->bytes[cpu->pc + 2];

        printf("  CPU Step [0x%04X] | %02X %02X %02X | %-6s ", cpu->pc, opcode, arg1, arg2,
               mnemonic_from_opcode(opcode));
        switch (opcode) {
        case OPCODE_LOADI: {
            printf("R%d, 0x%02X\n", arg1, arg2);
        } break;
        case OPCODE_LOAD: {
            printf("R%d\t;; R%d = MEM[R6:R7] \n", arg1, arg1);
        } break;
        case OPCODE_STORE: {
            printf("R%d\t;; MEM[R6:R7] = R%d\n", arg1, arg1);
        } break;
        case OPCODE_INC:
        case OPCODE_DEC: {
            printf("R%d\n", arg1);
        } break;
        case OPCODE_JMP:
        case OPCODE_JZ:
        case OPCODE_JNZ: {
            printf("0x%04X\n", ((arg1 << 8) | arg2));
        } break;
        case OPCODE_HALT: printf("\n"); break;
        default: {
            printf("R%d, R%d\n", arg1, arg2);
        }
        }
    } while ((++step < max_steps) && cpu_step(cpu, memory));
    return step <= max_steps;
}

const char* mnemonic_from_opcode(OpCode opcode) {
    for (size_t i = 0; i < OPCODE_COUNT; ++i) {
        if (opcode == opcode_table[i].opcode) {
            return opcode_table[i].name;
        }
    }
    return "UNKNOWN";
}

OpCode opcode_from_mnemonic(const char* mnemonic) {
    for (size_t i = 0; i < OPCODE_COUNT; ++i) {
        if (strcmp(mnemonic, opcode_table[i].name) == 0) {
            return opcode_table[i].opcode;
        }
    }
    return OPCODE_UNKNOWN;
}
