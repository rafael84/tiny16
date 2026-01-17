#include <assert.h>
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

    uint16_t pair0 = ((uint16_t)cpu->R[0] << 8) | cpu->R[1];
    uint16_t pair1 = ((uint16_t)cpu->R[2] << 8) | cpu->R[3];
    uint16_t pair2 = ((uint16_t)cpu->R[4] << 8) | cpu->R[5];
    uint16_t pair3 = ((uint16_t)cpu->R[6] << 8) | cpu->R[7];

    for (int i = 0; i < TINY16_CPU_REGISTERS; ++i) {
        printf("  --- R%d ---", i);
    }
    printf(" | R0:R1=0x%04X\n", pair0);

    for (int i = 0; i < TINY16_CPU_REGISTERS; ++i) {
        printf("  0b" TINY16_BIN8_FMT, TINY16_BIN8(cpu->R[i]));
    }
    printf(" | R2:R3=0x%04X\n", pair1);

    for (int i = 0; i < TINY16_CPU_REGISTERS; ++i) {
        printf("        0x%02X", cpu->R[i]);
    }
    printf(" | R4:R5=0x%04X\n", pair2);

    for (int i = 0; i < TINY16_CPU_REGISTERS; ++i) {
        printf("%12d", cpu->R[i]);
    }
    printf(" | R6:R7=0x%04X\n\n", pair3);

    uint8_t C = TINY16_CPU_FLAG(cpu->flags, TINY16_CPU_FLAG_C);
    uint8_t Z = TINY16_CPU_FLAG(cpu->flags, TINY16_CPU_FLAG_Z);

    printf("  ------- PC -------  ------- SP -------  | Flags\n");
    printf("  0b" TINY16_BIN16_FMT "  0b" TINY16_BIN16_FMT "  | CF  %d\n", TINY16_BIN16(cpu->pc),
           TINY16_BIN16(cpu->sp), C);
    printf("              0x%04X              0x%04X  | ZF  %d\n", cpu->pc, cpu->sp, Z);
    printf("%20d%20d  |\n", cpu->pc, cpu->sp);
}

#define TINY16_CPU_PUSH(cpu, mem_ctx, mem_write, value8)                                           \
    do {                                                                                           \
        uint16_t addr = cpu->sp - 1;                                                               \
        if (addr < TINY16_MEMORY_STACK_BEGIN) {                                                    \
            fprintf(stderr, "[CRITICAL] Stack overflow: 0x%04X (valid range is 0x%02X-0x%02X)\n",  \
                    addr, TINY16_MEMORY_STACK_BEGIN, TINY16_MEMORY_STACK_END);                     \
            return false;                                                                          \
        }                                                                                          \
        mem_write(mem_ctx, cpu->sp--, (value8));                                                   \
    } while (0)

#define TINY16_CPU_POP(cpu, mem_ctx, mem_read, out8)                                               \
    do {                                                                                           \
        uint16_t addr = cpu->sp + 1;                                                               \
        if (addr > TINY16_MEMORY_STACK_END) {                                                      \
            fprintf(stderr, "[CRITICAL] Stack underflow: 0x%04X (valid range is 0x%02X-0x%02X)\n", \
                    addr, TINY16_MEMORY_STACK_BEGIN, TINY16_MEMORY_STACK_END);                     \
            return false;                                                                          \
        }                                                                                          \
        out8 = mem_read(mem_ctx, ++cpu->sp);                                                       \
    } while (0)

#define TINY16_CPU_PUSH16(cpu, mem_ctx, mem_write, value16)                                        \
    do {                                                                                           \
        TINY16_CPU_PUSH(cpu, mem_ctx, mem_write, (value16 >> 8));                                  \
        TINY16_CPU_PUSH(cpu, mem_ctx, mem_write, (value16 & 0xFF));                                \
    } while (0)

#define TINY16_CPU_POP16(cpu, mem_ctx, mem_read, out16)                                            \
    do {                                                                                           \
        uint8_t lo, hi;                                                                            \
        TINY16_CPU_POP(cpu, mem_ctx, mem_read, lo);                                                \
        TINY16_CPU_POP(cpu, mem_ctx, mem_read, hi);                                                \
        out16 = ((uint16_t)hi << 8) | lo;                                                          \
    } while (0)

#define TINY16_CPU_TRACE_BUFFER_SIZE 40
static char tiny16_cpu_trace_buffer[TINY16_CPU_TRACE_BUFFER_SIZE];
static const char* TINY16_CPU_PAIR_NAMES[] = {"R0:R1", "R2:R3", "R4:R5", "R6:R7"};

void tiny16_cpu_trace(uint16_t addr, Tiny16OpCode opcode, uint8_t arg1, uint8_t arg2) {

    printf("    0x%04X | %02X %02X %02X | %-8s", addr, opcode, arg1, arg2,
           tiny16_mnemonic_from_opcode(opcode));

    switch (opcode) {
    case TINY16_OPCODE_LOADI:
        snprintf(tiny16_cpu_trace_buffer, TINY16_CPU_TRACE_BUFFER_SIZE, "R%d, 0x%02X", arg1, arg2);
        break;
    case TINY16_OPCODE_LOAD:
    case TINY16_OPCODE_STORE: {
        uint8_t reg = (arg1 >> 5) & 0x7;
        Tiny16AddrMode mode = (arg1 >> 3) & 0x3;
        uint8_t pair = (arg1 >> 1) & 0x3;
        static const char* mode_suffix[] = {"", "+", "-", ""};
        if (mode == TINY16_ADDR_MODE_OFFSET) {
            snprintf(tiny16_cpu_trace_buffer, TINY16_CPU_TRACE_BUFFER_SIZE, "R%d, [%s + 0x%02X]",
                     reg, TINY16_CPU_PAIR_NAMES[pair], arg2);
        } else {
            snprintf(tiny16_cpu_trace_buffer, TINY16_CPU_TRACE_BUFFER_SIZE, "R%d, [%s]%s", reg,
                     TINY16_CPU_PAIR_NAMES[pair], mode_suffix[mode]);
        }
    } break;
    case TINY16_OPCODE_INC:
    case TINY16_OPCODE_DEC:
    case TINY16_OPCODE_PUSH:
    case TINY16_OPCODE_POP:
        snprintf(tiny16_cpu_trace_buffer, TINY16_CPU_TRACE_BUFFER_SIZE, "R%d", arg1);
        break;
    case TINY16_OPCODE_MOVSPR:
    case TINY16_OPCODE_MOVRSP:
        snprintf(tiny16_cpu_trace_buffer, TINY16_CPU_TRACE_BUFFER_SIZE, "%s",
                 arg1 < 4 ? TINY16_CPU_PAIR_NAMES[arg1] : "INVALID");
        break;
    case TINY16_OPCODE_JMP:
    case TINY16_OPCODE_JZ:
    case TINY16_OPCODE_JNZ:
    case TINY16_OPCODE_JC:
    case TINY16_OPCODE_JNC:
    case TINY16_OPCODE_CALL:
        snprintf(tiny16_cpu_trace_buffer, TINY16_CPU_TRACE_BUFFER_SIZE, "0x%04X",
                 ((arg1 << 8) | arg2));
        break;
    case TINY16_OPCODE_RET:
    case TINY16_OPCODE_HALT:
        tiny16_cpu_trace_buffer[0] = '\0';
        break;
    default:
        snprintf(tiny16_cpu_trace_buffer, TINY16_CPU_TRACE_BUFFER_SIZE, "R%d, R%d", arg1, arg2);
    }

    printf("%-24s\t", tiny16_cpu_trace_buffer);
}

bool tiny16_cpu_step(Tiny16CPU* cpu, void* memory_context, tiny16_mem_read_fn memory_read,
                     tiny16_mem_write_fn memory_write) {

    if (cpu->pc < TINY16_MEMORY_CODE_BEGIN || cpu->pc + 2 >= TINY16_MEMORY_CODE_END) {
        fprintf(stderr, "[CRITICAL] PC left code segment: 0x%04X (0x%04X-0x%04X)\n", cpu->pc,
                TINY16_MEMORY_CODE_BEGIN, TINY16_MEMORY_CODE_END);
        return false;
    }

    if (tiny16_cpu_tracing) {
        Tiny16OpCode opcode = memory_read(memory_context, cpu->pc);
        uint16_t arg1 = memory_read(memory_context, cpu->pc + 1);
        uint16_t arg2 = memory_read(memory_context, cpu->pc + 2);
        tiny16_cpu_trace(cpu->pc, opcode, arg1, arg2);
    }

    uint16_t opcode = memory_read(memory_context, cpu->pc++);
    uint16_t arg1 = memory_read(memory_context, cpu->pc++);
    uint16_t arg2 = memory_read(memory_context, cpu->pc++);

    switch (opcode) {
    case TINY16_OPCODE_LOADI:
        cpu->R[arg1] = arg2;
        break;

    case TINY16_OPCODE_LOAD:
    case TINY16_OPCODE_STORE: {
        //
        // BYTE 0 : OPCODE   (LOAD / STORE)
        // BYTE 1 : REG | MODE | PAIR
        // BYTE 2 : AUX (offset or 0)
        //
        // BYTE 1  bits 7–5 : REG    (Rd or Rs)
        //         bits 4–3 : MODE   (addressing mode)
        //         bits 2–1 : PAIR   (00–11)
        //         bit  0   : unused (0)
        //

        uint8_t reg = (arg1 >> 5) & 0x7;
        Tiny16AddrMode mode = (arg1 >> 3) & 0x3;
        uint8_t pair = (arg1 >> 1) & 0x3;

        // PAIR: 0=R0:R1, 1=R2:R3, 2=R4:R5, 3=R6:R7
        uint8_t* hi = &cpu->R[pair * 2];     // even (high byte)
        uint8_t* lo = &cpu->R[pair * 2 + 1]; // odd (low byte)

        uint16_t addr = ((uint16_t)(*hi << 8)) | *lo;
        uint16_t ea = addr; // effective address

        if (mode == TINY16_ADDR_MODE_OFFSET) ea = addr + (uint8_t)arg2;

        switch (opcode) {
        case TINY16_OPCODE_LOAD:
            cpu->R[reg] = memory_read(memory_context, ea);
            break;
        case TINY16_OPCODE_STORE:
            memory_write(memory_context, ea, cpu->R[reg]);
            break;
        default:
            assert(0 && "unreachable");
        }

        if (mode == TINY16_ADDR_MODE_INC) addr++;
        if (mode == TINY16_ADDR_MODE_DEC) addr--;

        *hi = addr >> 8;
        *lo = addr & 0xFF;

    } break;

    case TINY16_OPCODE_MOV:
        cpu->R[arg1] = cpu->R[arg2];
        break;

    case TINY16_OPCODE_ADD: {
        uint16_t a = cpu->R[arg1];
        uint16_t b = cpu->R[arg2];
        uint16_t res = a + b;
        cpu->R[arg1] = res & UINT8_MAX;
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, res > 0xFF);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
    } break;

    case TINY16_OPCODE_ADC: {
        uint16_t a = cpu->R[arg1];
        uint16_t b = cpu->R[arg2];
        uint16_t res = a + b + TINY16_CPU_FLAG(cpu->flags, TINY16_CPU_FLAG_C);
        cpu->R[arg1] = res & UINT8_MAX;
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, res > 0xFF);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
    } break;

    case TINY16_OPCODE_SUB: {
        uint16_t a = cpu->R[arg1];
        uint16_t b = cpu->R[arg2];
        uint16_t res = a - b;
        cpu->R[arg1] = res & UINT8_MAX;
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, a < b);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
    } break;

    case TINY16_OPCODE_SBC: {
        uint16_t a = cpu->R[arg1];
        uint16_t b = cpu->R[arg2];
        uint16_t res = a - b - TINY16_CPU_FLAG(cpu->flags, TINY16_CPU_FLAG_C);
        cpu->R[arg1] = res & UINT8_MAX;
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, res > 0xFF);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
    } break;

    case TINY16_OPCODE_INC:
        cpu->R[arg1] = (cpu->R[arg1] + 1) & 0xFF;
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, 0);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
        break;

    case TINY16_OPCODE_DEC:
        cpu->R[arg1] = (cpu->R[arg1] - 1) & 0xFF;
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, 0);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
        break;

    case TINY16_OPCODE_AND:
        cpu->R[arg1] = cpu->R[arg1] & cpu->R[arg2];
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, 0);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
        break;

    case TINY16_OPCODE_OR:
        cpu->R[arg1] = cpu->R[arg1] | cpu->R[arg2];
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, 0);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
        break;

    case TINY16_OPCODE_XOR:
        cpu->R[arg1] = cpu->R[arg1] ^ cpu->R[arg2];
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, 0);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
        break;

    case TINY16_OPCODE_CMP: {
        uint16_t a = cpu->R[arg1];
        uint16_t b = cpu->R[arg2];
        uint16_t tmp = (a - b) & 0xFF;
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, a < b);
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, tmp == 0);
    } break;

    case TINY16_OPCODE_SHL:
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, (cpu->R[arg1] & 0x80) >> 7);
        cpu->R[arg1] <<= 1u;
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
        break;

    case TINY16_OPCODE_SHR:
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_C, cpu->R[arg1] & 0x01);
        cpu->R[arg1] >>= 1u;
        TINY16_CPU_SET_FLAG(cpu, TINY16_CPU_FLAG_Z, cpu->R[arg1] == 0);
        break;

    case TINY16_OPCODE_PUSH:
        TINY16_CPU_PUSH(cpu, memory_context, memory_write, cpu->R[arg1]);
        break;

    case TINY16_OPCODE_POP:
        TINY16_CPU_POP(cpu, memory_context, memory_read, cpu->R[arg1]);
        break;

    case TINY16_OPCODE_MOVSPR: {
        uint8_t hi = arg1 * 2;
        uint8_t lo = arg1 * 2 + 1;
        cpu->R[hi] = (cpu->sp >> 8) & 0xFF;
        cpu->R[lo] = cpu->sp & 0xFF;
    } break;

    case TINY16_OPCODE_MOVRSP: {
        uint8_t hi = arg1 * 2;
        uint8_t lo = arg1 * 2 + 1;
        cpu->sp = ((uint16_t)cpu->R[hi] << 8) | cpu->R[lo];
    } break;

    case TINY16_OPCODE_JMP:
        cpu->pc = ((uint16_t)arg1 << 8) | arg2;
        break;

    case TINY16_OPCODE_JZ:
        if (cpu->flags & (1u << TINY16_CPU_FLAG_Z)) {
            cpu->pc = ((uint16_t)arg1 << 8) | arg2;
        }
        break;

    case TINY16_OPCODE_JNZ:
        if (!(cpu->flags & (1u << TINY16_CPU_FLAG_Z))) {
            cpu->pc = ((uint16_t)arg1 << 8) | arg2;
        }
        break;

    case TINY16_OPCODE_JC:
        if (cpu->flags & (1u << TINY16_CPU_FLAG_C)) {
            cpu->pc = ((uint16_t)arg1 << 8) | arg2;
        }
        break;

    case TINY16_OPCODE_JNC:
        if (!(cpu->flags & (1u << TINY16_CPU_FLAG_C))) {
            cpu->pc = ((uint16_t)arg1 << 8) | arg2;
        }
        break;

    case TINY16_OPCODE_CALL: {
        uint16_t ret = cpu->pc;
        TINY16_CPU_PUSH16(cpu, memory_context, memory_write, ret);
        cpu->pc = ((uint16_t)arg1 << 8) | arg2;
    } break;

    case TINY16_OPCODE_RET: {
        uint16_t ret;
        TINY16_CPU_POP16(cpu, memory_context, memory_read, ret);
        cpu->pc = ret;
    } break;

    case TINY16_OPCODE_HALT:
        return false;
        break;

    default:
        fprintf(stderr, "[CRITICAL] Opcode not found: %X\n", opcode);
        return false;
    }

    if (tiny16_cpu_tracing) {
        uint8_t C = TINY16_CPU_FLAG(cpu->flags, TINY16_CPU_FLAG_C);
        uint8_t Z = TINY16_CPU_FLAG(cpu->flags, TINY16_CPU_FLAG_Z);
        printf(" |> C=%d Z=%d |", C, Z);
        for (uint8_t i = 0; i < TINY16_CPU_REGISTERS; ++i) {
            printf(" R%d=%02X", i, cpu->R[i]);
        }
        printf("\n");
    }

    return true;
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
        if (strcasecmp(mnemonic, opcode_table[i].name) == 0) {
            return opcode_table[i].opcode;
        }
    }
    return TINY16_OPCODE_UNKNOWN;
}
