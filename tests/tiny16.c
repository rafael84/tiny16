#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define TINY16_DEVELOPMENT_MODE

#include "cpu.c"
#include "cpu.h"
#include "memory.c"
#include "memory.h"

static Memory memory = {0};
static CPU cpu = {0};

void test_arithmetic(CPU* cpu, Memory* memory);
void test_borrow(CPU* cpu, Memory* memory);
void test_inc_dec_wraparound(CPU* cpu, Memory* memory);
void test_logic_ops(CPU* cpu, Memory* memory);
void test_load_store_indirect(CPU* cpu, Memory* memory);
void test_jz_conditional_jump(CPU* cpu, Memory* memory);
void test_jnz_mirror_test(CPU* cpu, Memory* memory);

#define TEST(name)                                                                                 \
    do {                                                                                           \
        printf("[TEST] %s\n", #name);                                                              \
        name(&cpu, &memory);                                                                       \
        printf("  OK\n");                                                                          \
    } while (0)

int main(void) {
    TEST(test_arithmetic);
    TEST(test_borrow);
    TEST(test_inc_dec_wraparound);
    TEST(test_logic_ops);
    TEST(test_load_store_indirect);
    TEST(test_jz_conditional_jump);
    TEST(test_jnz_mirror_test);
    return 0;
}

#define ASM(opcode, arg1, arg2)                                                                    \
    do {                                                                                           \
        memory->bytes[code++] = opcode;                                                            \
        memory->bytes[code++] = arg1;                                                              \
        memory->bytes[code++] = arg2;                                                              \
    } while (0)

#define RUN(...)                                                                                   \
    do {                                                                                           \
        cpu_reset(cpu);                                                                            \
        memory_reset(memory);                                                                      \
        int code = cpu->pc;                                                                        \
        __VA_OPT__(;) __VA_ARGS__;                                                                 \
        if (!cpu_exec(cpu, memory, 20)) {                                                          \
            puts("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");     \
            cpu_print(cpu);                                                                        \
            memory_print(memory, false);                                                           \
        }                                                                                          \
    } while (0)

#define FLAG(cpu, f) ((cpu)->flags & (1u << (f)))

void test_arithmetic(CPU* cpu, Memory* memory) {
    RUN(ASM(OPCODE_LOADI, 0, 0xFF); // R0 = 255
        ASM(OPCODE_LOADI, 1, 0x01); // R1 = 1
        ASM(OPCODE_ADD, 0, 1);      // R0 = 0, Z=1, C=1
        ASM(OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 0);
    assert(FLAG(cpu, CPU_FLAG_Z));
    assert(FLAG(cpu, CPU_FLAG_C));
}

void test_borrow(CPU* cpu, Memory* memory) {
    RUN(ASM(OPCODE_LOADI, 0, 0x00); // R0 = 0
        ASM(OPCODE_LOADI, 1, 0x01); // R1 = 1
        ASM(OPCODE_SUB, 0, 1);      // R0 = 255, borrow
        ASM(OPCODE_HALT, 0, 0));
    assert(cpu->R[0] == 0xFF);
    assert(!(FLAG(cpu, CPU_FLAG_Z)));
    assert(FLAG(cpu, CPU_FLAG_C));
}

void test_inc_dec_wraparound(CPU* cpu, Memory* memory) {
    RUN(ASM(OPCODE_LOADI, 0, 0xFF); // R0 = 255
        ASM(OPCODE_INC, 0, 0);      // R0 = 0, Z=1
        ASM(OPCODE_DEC, 0, 0);      // R0 = 255, Z=0
        ASM(OPCODE_HALT, 0, 0));
    assert(cpu->R[0] == 0xFF);
}

void test_logic_ops(CPU* cpu, Memory* memory) {
    RUN(ASM(OPCODE_LOADI, 0, 0xF0); // 11110000
        ASM(OPCODE_LOADI, 1, 0x0F); // 00001111

        ASM(OPCODE_AND, 0, 1); // R0 = 0x00, Z=1
        ASM(OPCODE_OR, 0, 1);  // R0 = 0x0F
        ASM(OPCODE_XOR, 0, 1); // R0 = 0x00, Z=1

        ASM(OPCODE_HALT, 0, 0));
    assert(cpu->R[0] == 0);
    assert(FLAG(cpu, CPU_FLAG_Z));
}

void test_load_store_indirect(CPU* cpu, Memory* memory) {
    RUN(ASM(OPCODE_LOADI, 6, 0x20); // R6 = 0x20
        ASM(OPCODE_LOADI, 7, 0x00); // R7 = 0x00  → ADDR = 0x2000

        ASM(OPCODE_LOADI, 0, 0xAB); // R0 = 0xAB
        ASM(OPCODE_STORE, 0, 0x00); // MEM[0x2000] = R0

        ASM(OPCODE_LOADI, 0, 0x00); // R0 = 0
        ASM(OPCODE_LOAD, 0, 0x00);  // R0 = MEM[0x2000]

        ASM(OPCODE_HALT, 0, 0));
    assert(cpu->R[0] == 0xAB);
}

void test_jz_conditional_jump(CPU* cpu, Memory* memory) {
    RUN(ASM(OPCODE_LOADI, 1, 3); // R1 = 3

        // loop @ 0x0003
        ASM(OPCODE_DEC, 1, 0);       // R1--
        ASM(OPCODE_JZ, 0x00, 0x0C);  // if Z, jump to HALT
        ASM(OPCODE_JMP, 0x00, 0x03); // jump back to DEC

        ASM(OPCODE_HALT, 0, 0););
    assert(cpu->R[1] == 0);
}

void test_jnz_mirror_test(CPU* cpu, Memory* memory) {
    RUN(ASM(OPCODE_LOADI, 1, 3);                            // R1 = 3
        ASM(OPCODE_DEC, 1, 0); ASM(OPCODE_JNZ, 0x00, 0x03); // loop while R1 != 0
        ASM(OPCODE_HALT, 0, 0););
    assert(cpu->R[1] == 0);
}

// memory.bytes[MEMORY_STACK_BEGIN] = 0xDE;
// for (int y = 0; y < MMIO_FRAMEBUFFER_SIZE_HEIGHT; ++y) {
//     for (int x = 0; x < MMIO_FRAMEBUFFER_SIZE_WIDTH; ++x) {
//         memory.bytes[MMIO_FRAMEBUFFER_ADDR(x, y)] = 7;
//     }
// }
