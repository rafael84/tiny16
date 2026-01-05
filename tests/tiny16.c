#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define TINY16_DEVELOPMENT_MODE
// #define TINY16_DEBUG_TRACE_CPU

#include "cpu.c"
#include "cpu.h"
#include "memory.c"
#include "memory.h"

static Tiny16Memory memory = {0};
static Tiny16CPU cpu = {0};

void tiny16_test_arithmetic(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_borrow(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_inc_dec_wraparound(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_logic_ops(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_load_store_indirect(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_jz_conditional_jump(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_jnz_mirror_test(Tiny16CPU* cpu, Tiny16Memory* memory);

#define TINY16_TEST(fn)                                                                            \
    do {                                                                                           \
        printf("[TINY16_TEST] %s\n", #fn);                                                         \
        fn(&cpu, &memory);                                                                         \
    } while (0)

int main(void) {
    TINY16_TEST(tiny16_test_arithmetic);
    TINY16_TEST(tiny16_test_borrow);
    TINY16_TEST(tiny16_test_inc_dec_wraparound);
    TINY16_TEST(tiny16_test_logic_ops);
    TINY16_TEST(tiny16_test_load_store_indirect);
    TINY16_TEST(tiny16_test_jz_conditional_jump);
    TINY16_TEST(tiny16_test_jnz_mirror_test);
    return 0;
}

#define TINY16_ASM(opcode, arg1, arg2)                                                             \
    do {                                                                                           \
        memory->bytes[code++] = opcode;                                                            \
        memory->bytes[code++] = arg1;                                                              \
        memory->bytes[code++] = arg2;                                                              \
    } while (0)

#define TINY16_TINY16_TEST_RUN(...)                                                                \
    do {                                                                                           \
        tiny16_cpu_reset(cpu);                                                                     \
        tiny16_memory_reset(memory);                                                               \
        int code = cpu->pc;                                                                        \
        __VA_ARGS__;                                                                               \
        if (!tiny16_cpu_exec(cpu, memory, 20)) {                                                   \
            puts("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");     \
            tiny16_cpu_print(cpu);                                                                 \
            tiny16_memory_print(memory, false);                                                    \
        }                                                                                          \
    } while (0)

void tiny16_test_arithmetic(Tiny16CPU* cpu, Tiny16Memory* memory) {
    TINY16_TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xFF); // R0 = 255
                           TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x01); // R1 = 1
                           TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);      // R0 = 0, Z=1, C=1
                           TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 0);
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_Z));
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_C));
}

void tiny16_test_borrow(Tiny16CPU* cpu, Tiny16Memory* memory) {
    TINY16_TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x00); // R0 = 0
                           TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x01); // R1 = 1
                           TINY16_ASM(TINY16_OPCODE_SUB, 0, 1);      // R0 = 255, borrow
                           TINY16_ASM(TINY16_OPCODE_HALT, 0, 0));
    assert(cpu->R[0] == 0xFF);
    assert(!(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_Z)));
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_C));
}

void tiny16_test_inc_dec_wraparound(Tiny16CPU* cpu, Tiny16Memory* memory) {
    TINY16_TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xFF); // R0 = 255
                           TINY16_ASM(TINY16_OPCODE_INC, 0, 0);      // R0 = 0, Z=1
                           TINY16_ASM(TINY16_OPCODE_DEC, 0, 0);      // R0 = 255, Z=0
                           TINY16_ASM(TINY16_OPCODE_HALT, 0, 0));
    assert(cpu->R[0] == 0xFF);
}

void tiny16_test_logic_ops(Tiny16CPU* cpu, Tiny16Memory* memory) {
    TINY16_TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xF0); // 11110000
                           TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x0F); // 00001111

                           TINY16_ASM(TINY16_OPCODE_AND, 0, 1); // R0 = 0x00, Z=1
                           TINY16_ASM(TINY16_OPCODE_OR, 0, 1);  // R0 = 0x0F
                           TINY16_ASM(TINY16_OPCODE_XOR, 0, 1); // R0 = 0x00, Z=1

                           TINY16_ASM(TINY16_OPCODE_HALT, 0, 0));
    assert(cpu->R[0] == 0);
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_Z));
}

void tiny16_test_load_store_indirect(Tiny16CPU* cpu, Tiny16Memory* memory) {
    TINY16_TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 6, 0x20); // R6 = 0x20
                           TINY16_ASM(TINY16_OPCODE_LOADI, 7, 0x00); // R7 = 0x00  → ADDR = 0x2000

                           TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xAB); // R0 = 0xAB
                           TINY16_ASM(TINY16_OPCODE_STORE, 0, 0x00); // MEM[0x2000] = R0

                           TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x00); // R0 = 0
                           TINY16_ASM(TINY16_OPCODE_LOAD, 0, 0x00);  // R0 = MEM[0x2000]

                           TINY16_ASM(TINY16_OPCODE_HALT, 0, 0));
    assert(cpu->R[0] == 0xAB);
}

#define ADDR16(instruction) (TINY16_MEMORY_CODE_BEGIN + instruction * 3)

void tiny16_test_jz_conditional_jump(Tiny16CPU* cpu, Tiny16Memory* memory) {
    TINY16_TINY16_TEST_RUN(
        /* 0x00 */ TINY16_ASM(TINY16_OPCODE_LOADI, 1, 3);          // R1 = 3
        /* 0x01 */ TINY16_ASM(TINY16_OPCODE_DEC, 1, 0);            // R1--
        /* 0x02 */ TINY16_ASM(TINY16_OPCODE_JZ, 0x00, ADDR16(4));  // if Z, jump to HALT
        /* 0x03 */ TINY16_ASM(TINY16_OPCODE_JMP, 0x00, ADDR16(1)); // jump back to DEC
        /* 0x04 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);           //
    );
    assert(cpu->R[1] == 0);
}

void tiny16_test_jnz_mirror_test(Tiny16CPU* cpu, Tiny16Memory* memory) {
    TINY16_TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 1, 3);          // R1 = 3
                           TINY16_ASM(TINY16_OPCODE_DEC, 1, 0);            // R1 = R1 - 1
                           TINY16_ASM(TINY16_OPCODE_JNZ, 0x00, ADDR16(1)); // loop while R1 != 0
                           TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);           //
    );
    assert(cpu->R[1] == 0);
}
