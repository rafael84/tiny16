#include <assert.h>
#include <stdint.h>
#include <stdio.h>

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
void tiny16_test_mov_register(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_shift_left(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_shift_right(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_shift_edge_cases(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_push_pop_basic(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_push_pop_multiple(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_all_registers(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_complex_arithmetic(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_nested_loops(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_zero_flag(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_carry_flag_propagation(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_memory_boundary(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_jump_backward_forward(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_xor_self_zeroing(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_address_registers_arithmetic(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_stack_pointer_behavior(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_conditional_jumps_not_taken(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_logic_identity_operations(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_chained_arithmetic_carry(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_shift_until_zero(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_stack_memory_access(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_sub_equal_values(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_max_steps_exceeded(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_call_ret_basic(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_call_ret_with_args(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_call_ret_nested(Tiny16CPU* cpu, Tiny16Memory* memory);
void tiny16_test_call_ret_preserves_registers(Tiny16CPU* cpu, Tiny16Memory* memory);

#define TINY16_TEST(fn)                                                                            \
    do {                                                                                           \
        printf(" ▸ %s", #fn);                                                                      \
        fflush(stdout);                                                                            \
        fn(&cpu, &memory);                                                                         \
        printf(" ✓\n");                                                                            \
    } while (0)

int main(void) {
    TINY16_TEST(tiny16_test_arithmetic);
    TINY16_TEST(tiny16_test_borrow);
    TINY16_TEST(tiny16_test_inc_dec_wraparound);
    TINY16_TEST(tiny16_test_logic_ops);
    TINY16_TEST(tiny16_test_load_store_indirect);
    TINY16_TEST(tiny16_test_jz_conditional_jump);
    TINY16_TEST(tiny16_test_jnz_mirror_test);
    TINY16_TEST(tiny16_test_mov_register);
    TINY16_TEST(tiny16_test_shift_left);
    TINY16_TEST(tiny16_test_shift_right);
    TINY16_TEST(tiny16_test_shift_edge_cases);
    TINY16_TEST(tiny16_test_push_pop_basic);
    TINY16_TEST(tiny16_test_push_pop_multiple);
    TINY16_TEST(tiny16_test_all_registers);
    TINY16_TEST(tiny16_test_complex_arithmetic);
    TINY16_TEST(tiny16_test_nested_loops);
    TINY16_TEST(tiny16_test_zero_flag);
    TINY16_TEST(tiny16_test_carry_flag_propagation);
    TINY16_TEST(tiny16_test_memory_boundary);
    TINY16_TEST(tiny16_test_jump_backward_forward);
    TINY16_TEST(tiny16_test_xor_self_zeroing);
    TINY16_TEST(tiny16_test_address_registers_arithmetic);
    TINY16_TEST(tiny16_test_stack_pointer_behavior);
    TINY16_TEST(tiny16_test_conditional_jumps_not_taken);
    TINY16_TEST(tiny16_test_logic_identity_operations);
    TINY16_TEST(tiny16_test_chained_arithmetic_carry);
    TINY16_TEST(tiny16_test_shift_until_zero);
    TINY16_TEST(tiny16_test_stack_memory_access);
    TINY16_TEST(tiny16_test_sub_equal_values);
    TINY16_TEST(tiny16_test_max_steps_exceeded);
    TINY16_TEST(tiny16_test_call_ret_basic);
    TINY16_TEST(tiny16_test_call_ret_with_args);
    TINY16_TEST(tiny16_test_call_ret_nested);
    TINY16_TEST(tiny16_test_call_ret_preserves_registers);
    return 0;
}

#define TINY16_ASM(opcode, arg1, arg2)                                                             \
    do {                                                                                           \
        memory->bytes[code++] = opcode;                                                            \
        memory->bytes[code++] = arg1;                                                              \
        memory->bytes[code++] = arg2;                                                              \
    } while (0)

#define TINY16_TEST_RUN(...)                                                                       \
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
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xFF); // R0 = 255
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x01); // R1 = 1
                    TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);      // R0 = 0, Z=1, C=1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 0);
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_Z));
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_C));
}

void tiny16_test_borrow(Tiny16CPU* cpu, Tiny16Memory* memory) {
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x00); // R0 = 0
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x01); // R1 = 1
                    TINY16_ASM(TINY16_OPCODE_SUB, 0, 1);      // R0 = 255, borrow
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0));
    assert(cpu->R[0] == 0xFF);
    assert(!(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_Z)));
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_C));
}

void tiny16_test_inc_dec_wraparound(Tiny16CPU* cpu, Tiny16Memory* memory) {
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xFF); // R0 = 255
                    TINY16_ASM(TINY16_OPCODE_INC, 0, 0);      // R0 = 0, Z=1
                    TINY16_ASM(TINY16_OPCODE_DEC, 0, 0);      // R0 = 255, Z=0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0));
    assert(cpu->R[0] == 0xFF);
}

void tiny16_test_logic_ops(Tiny16CPU* cpu, Tiny16Memory* memory) {
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xF0); // 11110000
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x0F); // 00001111

                    TINY16_ASM(TINY16_OPCODE_AND, 0, 1); // R0 = 0x00, Z=1
                    TINY16_ASM(TINY16_OPCODE_OR, 0, 1);  // R0 = 0x0F
                    TINY16_ASM(TINY16_OPCODE_XOR, 0, 1); // R0 = 0x00, Z=1

                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0));
    assert(cpu->R[0] == 0);
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_Z));
}

void tiny16_test_load_store_indirect(Tiny16CPU* cpu, Tiny16Memory* memory) {
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 6, 0x20); // R6 = 0x20
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
    TINY16_TEST_RUN(
        /* 0x00 */ TINY16_ASM(TINY16_OPCODE_LOADI, 1, 3);          // R1 = 3
        /* 0x01 */ TINY16_ASM(TINY16_OPCODE_DEC, 1, 0);            // R1--
        /* 0x02 */ TINY16_ASM(TINY16_OPCODE_JZ, 0x00, ADDR16(4));  // if Z, jump to HALT
        /* 0x03 */ TINY16_ASM(TINY16_OPCODE_JMP, 0x00, ADDR16(1)); // jump back to DEC
        /* 0x04 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);           //
    );
    assert(cpu->R[1] == 0);
}

void tiny16_test_jnz_mirror_test(Tiny16CPU* cpu, Tiny16Memory* memory) {
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 1, 3);          // R1 = 3
                    TINY16_ASM(TINY16_OPCODE_DEC, 1, 0);            // R1 = R1 - 1
                    TINY16_ASM(TINY16_OPCODE_JNZ, 0x00, ADDR16(1)); // loop while R1 != 0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);           //
    );
    assert(cpu->R[1] == 0);
}

void tiny16_test_mov_register(Tiny16CPU* cpu, Tiny16Memory* memory) {
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xAB); // R0 = 0xAB
                    TINY16_ASM(TINY16_OPCODE_MOV, 1, 0);      // R1 = R0
                    TINY16_ASM(TINY16_OPCODE_MOV, 2, 1);      // R2 = R1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 0xAB);
    assert(cpu->R[1] == 0xAB);
    assert(cpu->R[2] == 0xAB);
}

void tiny16_test_shift_left(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test SHL: shift left logical
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x01); // R0 = 0b00000001
                    TINY16_ASM(TINY16_OPCODE_SHL, 0, 0);      // R0 = 0b00000010 (2)
                    TINY16_ASM(TINY16_OPCODE_SHL, 0, 0);      // R0 = 0b00000100 (4)
                    TINY16_ASM(TINY16_OPCODE_SHL, 0, 0);      // R0 = 0b00001000 (8)
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 8);
}

void tiny16_test_shift_right(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test SHR: shift right logical
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x80); // R0 = 0b10000000 (128)
                    TINY16_ASM(TINY16_OPCODE_SHR, 0, 0);      // R0 = 0b01000000 (64)
                    TINY16_ASM(TINY16_OPCODE_SHR, 0, 0);      // R0 = 0b00100000 (32)
                    TINY16_ASM(TINY16_OPCODE_SHR, 0, 0);      // R0 = 0b00010000 (16)
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 16);
}

void tiny16_test_shift_edge_cases(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test shifting out bits and wrapping
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xFF); // R0 = 0b11111111
                    TINY16_ASM(TINY16_OPCODE_SHL, 0, 0);      // R0 = 0b11111110, carry = 1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 0xFE);
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_C)); // carry should be set

    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0xFF); // R1 = 0b11111111
                    TINY16_ASM(TINY16_OPCODE_SHR, 1, 0);      // R1 = 0b01111111, carry = 1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[1] == 0x7F);
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_C)); // carry should be set
}

void tiny16_test_push_pop_basic(Tiny16CPU* cpu, Tiny16Memory* memory) {
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x42); // R0 = 0x42
                    TINY16_ASM(TINY16_OPCODE_PUSH, 0, 0);     // push R0
                    TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x00); // R0 = 0
                    TINY16_ASM(TINY16_OPCODE_POP, 0, 0);      // pop to R0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 0x42);
}

void tiny16_test_push_pop_multiple(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test LIFO stack behavior with multiple values
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x11); // R0 = 0x11
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x22); // R1 = 0x22
                    TINY16_ASM(TINY16_OPCODE_LOADI, 2, 0x33); // R2 = 0x33
                    TINY16_ASM(TINY16_OPCODE_PUSH, 0, 0);     // push R0
                    TINY16_ASM(TINY16_OPCODE_PUSH, 1, 0);     // push R1
                    TINY16_ASM(TINY16_OPCODE_PUSH, 2, 0);     // push R2
                    TINY16_ASM(TINY16_OPCODE_POP, 3, 0);      // pop to R3 (should be 0x33)
                    TINY16_ASM(TINY16_OPCODE_POP, 4, 0);      // pop to R4 (should be 0x22)
                    TINY16_ASM(TINY16_OPCODE_POP, 5, 0);      // pop to R5 (should be 0x11)
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[3] == 0x33);
    assert(cpu->R[4] == 0x22);
    assert(cpu->R[5] == 0x11);
}

void tiny16_test_all_registers(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test that all 8 registers work independently
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x00); // R0 = 0
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x11); // R1 = 17
                    TINY16_ASM(TINY16_OPCODE_LOADI, 2, 0x22); // R2 = 34
                    TINY16_ASM(TINY16_OPCODE_LOADI, 3, 0x33); // R3 = 51
                    TINY16_ASM(TINY16_OPCODE_LOADI, 4, 0x44); // R4 = 68
                    TINY16_ASM(TINY16_OPCODE_LOADI, 5, 0x55); // R5 = 85
                    TINY16_ASM(TINY16_OPCODE_LOADI, 6, 0x66); // R6 = 102
                    TINY16_ASM(TINY16_OPCODE_LOADI, 7, 0x77); // R7 = 119
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 0x00);
    assert(cpu->R[1] == 0x11);
    assert(cpu->R[2] == 0x22);
    assert(cpu->R[3] == 0x33);
    assert(cpu->R[4] == 0x44);
    assert(cpu->R[5] == 0x55);
    assert(cpu->R[6] == 0x66);
    assert(cpu->R[7] == 0x77);
}

void tiny16_test_complex_arithmetic(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test: (10 + 20) * 2 = 60 using shifts for multiplication
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 10); // R0 = 10
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 20); // R1 = 20
                    TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);    // R0 = 30
                    TINY16_ASM(TINY16_OPCODE_SHL, 0, 0);    // R0 = 60 (multiply by 2)
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 60);
}

void tiny16_test_nested_loops(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test nested loops: outer loop 2 times, inner loop 2 times = 4 total iterations
    // R0 will count total iterations
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0);          // R0 = 0 (counter)
        /* 1 */ TINY16_ASM(TINY16_OPCODE_LOADI, 1, 2);          // R1 = 2 (outer)
        /* 2 */ TINY16_ASM(TINY16_OPCODE_LOADI, 2, 2);          // R2 = 2 (inner init)
        /* 3 */ TINY16_ASM(TINY16_OPCODE_MOV, 3, 2);            // R3 = R2 (inner counter)
        /* 4 */ TINY16_ASM(TINY16_OPCODE_INC, 0, 0);            // R0++
        /* 5 */ TINY16_ASM(TINY16_OPCODE_DEC, 3, 0);            // R3--
        /* 6 */ TINY16_ASM(TINY16_OPCODE_JNZ, 0x00, ADDR16(4)); // if R3 != 0, loop inner
        /* 7 */ TINY16_ASM(TINY16_OPCODE_DEC, 1, 0);            // R1--
        /* 8 */ TINY16_ASM(TINY16_OPCODE_JNZ, 0x00, ADDR16(3)); // if R1 != 0, loop outer
        /* 9 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);           //
    );
    assert(cpu->R[0] == 4); // 2 * 2 = 4 iterations
}

void tiny16_test_zero_flag(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test that zero flag is set/cleared correctly
    // Z flag is only set by ALU operations, not LOADI
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x05); // R0 = 5
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x05); // R1 = 5
                    TINY16_ASM(TINY16_OPCODE_SUB, 0, 1);      // R0 = 0, Z should be set
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 0);
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_Z));

    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x01); // R0 = 1
                    TINY16_ASM(TINY16_OPCODE_DEC, 0, 0);      // R0 = 0, Z should be set
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 0);
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_Z));

    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x05); // R0 = 5
                    TINY16_ASM(TINY16_OPCODE_INC, 0, 0);      // R0 = 6, Z should be clear
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 6);
    assert(!TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_Z));
}

void tiny16_test_carry_flag_propagation(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test carry flag in various operations
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x80); // R0 = 128
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x80); // R1 = 128
                    TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);      // R0 = 0, C = 1 (overflow)
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 0);
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_Z));
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_C));
}

void tiny16_test_memory_boundary(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test loading and storing at various memory locations in data segment
    TINY16_TEST_RUN(
        TINY16_ASM(TINY16_OPCODE_LOADI, 6, 0x20); // R6:R7 = 0x2000 (data segment start)
        TINY16_ASM(TINY16_OPCODE_LOADI, 7, 0x00); //
        TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xAA); // R0 = 0xAA
        TINY16_ASM(TINY16_OPCODE_STORE, 0, 0);    // MEM[0x2000] = 0xAA

        TINY16_ASM(TINY16_OPCODE_LOADI, 6, 0x30); // R6:R7 = 0x3000 (middle of data segment)
        TINY16_ASM(TINY16_OPCODE_LOADI, 7, 0x00); //
        TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0xBB); // R1 = 0xBB
        TINY16_ASM(TINY16_OPCODE_STORE, 1, 0);    // MEM[0x3000] = 0xBB

        TINY16_ASM(TINY16_OPCODE_LOADI, 6, 0x20); // R6:R7 = 0x2000
        TINY16_ASM(TINY16_OPCODE_LOADI, 7, 0x00); //
        TINY16_ASM(TINY16_OPCODE_LOAD, 2, 0);     // R2 = MEM[0x2000]

        TINY16_ASM(TINY16_OPCODE_LOADI, 6, 0x30); // R6:R7 = 0x3000
        TINY16_ASM(TINY16_OPCODE_LOADI, 7, 0x00); //
        TINY16_ASM(TINY16_OPCODE_LOAD, 3, 0);     // R3 = MEM[0x3000]

        TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[2] == 0xAA);
    assert(cpu->R[3] == 0xBB);
}

void tiny16_test_jump_backward_forward(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test forward and backward jumps
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0);          // R0 = 0
        /* 1 */ TINY16_ASM(TINY16_OPCODE_JMP, 0x00, ADDR16(3)); // jump forward to instruction 3
        /* 2 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 99);         // this should be skipped
        /* 3 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 42);         // R0 = 42
        /* 4 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);           //
    );
    assert(cpu->R[0] == 42); // Should be 42, not 0 or 99
}

void tiny16_test_xor_self_zeroing(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test XOR R, R is a common idiom to zero a register efficiently
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xAB); // R0 = 0xAB
                    TINY16_ASM(TINY16_OPCODE_XOR, 0, 0);      // R0 = R0 XOR R0 = 0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 0);
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_Z));
}

void tiny16_test_address_registers_arithmetic(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test that R6 and R7 can be used for arithmetic, not just addressing
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 6, 10); // R6 = 10
                    TINY16_ASM(TINY16_OPCODE_LOADI, 7, 5);  // R7 = 5
                    TINY16_ASM(TINY16_OPCODE_ADD, 6, 7);    // R6 = R6 + R7 = 15
                    TINY16_ASM(TINY16_OPCODE_SUB, 7, 7);    // R7 = R7 - R7 = 0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[6] == 15);
    assert(cpu->R[7] == 0);
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_Z));
}

void tiny16_test_stack_pointer_behavior(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Verify stack pointer changes correctly with PUSH and POP
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x42); // R0 = 0x42
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    uint16_t initial_sp = cpu->sp;

    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x11); // R0 = 0x11
                    TINY16_ASM(TINY16_OPCODE_PUSH, 0, 0);     // push R0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->sp == initial_sp - 1); // SP should decrement

    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x11); // R0 = 0x11
                    TINY16_ASM(TINY16_OPCODE_PUSH, 0, 0);     // push R0
                    TINY16_ASM(TINY16_OPCODE_POP, 1, 0);      // pop to R1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->sp == initial_sp); // SP should be back to initial
}

void tiny16_test_conditional_jumps_not_taken(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test that conditional jumps fall through when condition is false
    // JZ should NOT jump when Z=0
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 1);         // R0 = 1
        /* 1 */ TINY16_ASM(TINY16_OPCODE_INC, 0, 0);           // R0 = 2, Z=0
        /* 2 */ TINY16_ASM(TINY16_OPCODE_JZ, 0x00, ADDR16(4)); // should NOT jump (Z=0)
        /* 3 */ TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x42);      // R1 = 0x42 (executed)
        /* 4 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);          //
    );
    assert(cpu->R[1] == 0x42); // Should have executed line 3

    // JNZ should NOT jump when Z=1
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 1);          // R0 = 1
        /* 1 */ TINY16_ASM(TINY16_OPCODE_DEC, 0, 0);            // R0 = 0, Z=1
        /* 2 */ TINY16_ASM(TINY16_OPCODE_JNZ, 0x00, ADDR16(4)); // should NOT jump (Z=1)
        /* 3 */ TINY16_ASM(TINY16_OPCODE_LOADI, 2, 0x99);       // R2 = 0x99 (executed)
        /* 4 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);           //
    );
    assert(cpu->R[2] == 0x99); // Should have executed line 3
}

void tiny16_test_logic_identity_operations(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test logic operations that should preserve or set specific values
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x55); // R0 = 0x55
                    TINY16_ASM(TINY16_OPCODE_OR, 0, 0);       // R0 OR R0 = R0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 0x55);

    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0xAA); // R1 = 0xAA
                    TINY16_ASM(TINY16_OPCODE_AND, 1, 1);      // R1 AND R1 = R1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[1] == 0xAA);

    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 2, 0xFF); // R2 = 0xFF
                    TINY16_ASM(TINY16_OPCODE_LOADI, 3, 0x00); // R3 = 0x00
                    TINY16_ASM(TINY16_OPCODE_AND, 2, 3);      // R2 AND 0 = 0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[2] == 0);
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_Z));

    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 4, 0x00); // R4 = 0x00
                    TINY16_ASM(TINY16_OPCODE_LOADI, 5, 0xFF); // R5 = 0xFF
                    TINY16_ASM(TINY16_OPCODE_OR, 4, 5);       // R4 OR 0xFF = 0xFF
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[4] == 0xFF);
}

void tiny16_test_chained_arithmetic_carry(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test multiple arithmetic operations and carry flag behavior
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xFF); // R0 = 255
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x01); // R1 = 1
                    TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);      // R0 = 0, C=1
                    TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);      // R0 = 1, C=0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 1);
    assert(!TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_C));
}

void tiny16_test_shift_until_zero(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test shifting a value repeatedly until it becomes zero
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x08); // R0 = 8 (0b00001000)
                    TINY16_ASM(TINY16_OPCODE_SHR, 0, 0);      // R0 = 4
                    TINY16_ASM(TINY16_OPCODE_SHR, 0, 0);      // R0 = 2
                    TINY16_ASM(TINY16_OPCODE_SHR, 0, 0);      // R0 = 1
                    TINY16_ASM(TINY16_OPCODE_SHR, 0, 0);      // R0 = 0, Z=1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 0);
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_Z));
}

void tiny16_test_stack_memory_access(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test loading and storing at stack segment addresses
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 6, 0x80); // R6:R7 = 0x8000 (stack begin)
                    TINY16_ASM(TINY16_OPCODE_LOADI, 7, 0x00); //
                    TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xCD); // R0 = 0xCD
                    TINY16_ASM(TINY16_OPCODE_STORE, 0, 0);    // MEM[0x8000] = 0xCD

                    TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x00); // R0 = 0
                    TINY16_ASM(TINY16_OPCODE_LOAD, 0, 0);     // R0 = MEM[0x8000]

                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 0xCD);
}

void tiny16_test_sub_equal_values(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test SUB with equal values: should give 0, set Z, clear C
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x42); // R0 = 0x42
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x42); // R1 = 0x42
                    TINY16_ASM(TINY16_OPCODE_SUB, 0, 1);      // R0 = 0, Z=1, C=0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(cpu->R[0] == 0);
    assert(TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_Z));
    assert(!TINY16_CPU_FLAG(cpu, TINY16_CPU_FLAG_C));
}

void tiny16_test_max_steps_exceeded(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test that execution stops when max_steps is reached (infinite loop)
    tiny16_cpu_reset(cpu);
    tiny16_memory_reset(memory);
    int code = cpu->pc;

    // Create an infinite loop
    TINY16_ASM(TINY16_OPCODE_JMP, 0x00, ADDR16(0)); // jump to itself

    // Execute with very low max_steps
    bool result = tiny16_cpu_exec(cpu, memory, 5);

    // Should return true (execution completes max_steps without error)
    assert(result);
    // PC should still be at the loop instruction (infinite loop continues)
    assert(cpu->pc == TINY16_MEMORY_CODE_BEGIN);
}

void tiny16_test_call_ret_basic(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test basic CALL/RET: call a subroutine that increments R0, then return
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 10);          // R0 = 10
        /* 1 */ TINY16_ASM(TINY16_OPCODE_CALL, 0x00, ADDR16(4)); // call subroutine at 4
        /* 2 */ TINY16_ASM(TINY16_OPCODE_INC, 0, 0);             // R0++ (after return)
        /* 3 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);            //
        /* 4 */ TINY16_ASM(TINY16_OPCODE_INC, 0, 0);             // subroutine: R0++
        /* 5 */ TINY16_ASM(TINY16_OPCODE_RET, 0, 0);             // return
    );
    assert(cpu->R[0] == 12); // 10 + 1 (in subroutine) + 1 (after return)
}

void tiny16_test_call_ret_with_args(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test CALL/RET with "arguments" passed through registers
    // Subroutine adds R1 to R0
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 5);           // R0 = 5
        /* 1 */ TINY16_ASM(TINY16_OPCODE_LOADI, 1, 7);           // R1 = 7
        /* 2 */ TINY16_ASM(TINY16_OPCODE_CALL, 0x00, ADDR16(5)); // call add_r1_to_r0
        /* 3 */ TINY16_ASM(TINY16_OPCODE_INC, 0, 0);             // R0++
        /* 4 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);            //
        /* 5 */ TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);             // add_r1_to_r0: R0 += R1
        /* 6 */ TINY16_ASM(TINY16_OPCODE_RET, 0, 0);             // return
    );
    assert(cpu->R[0] == 13); // 5 + 7 + 1 = 13
}

void tiny16_test_call_ret_nested(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test nested CALL: main -> func_a -> func_b
    // func_b increments R0, func_a calls func_b twice
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0);           // R0 = 0
        /* 1 */ TINY16_ASM(TINY16_OPCODE_CALL, 0x00, ADDR16(4)); // call func_a
        /* 2 */ TINY16_ASM(TINY16_OPCODE_INC, 0, 0);             // R0++ (after return)
        /* 3 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);            //
        /* 4 */ TINY16_ASM(TINY16_OPCODE_CALL, 0x00, ADDR16(7)); // func_a: call func_b
        /* 5 */ TINY16_ASM(TINY16_OPCODE_CALL, 0x00, ADDR16(7)); // func_a: call func_b again
        /* 6 */ TINY16_ASM(TINY16_OPCODE_RET, 0, 0);             // func_a: return
        /* 7 */ TINY16_ASM(TINY16_OPCODE_INC, 0, 0);             // func_b: R0++
        /* 8 */ TINY16_ASM(TINY16_OPCODE_RET, 0, 0);             // func_b: return
    );
    assert(cpu->R[0] == 3); // 0 + 1 (func_b call 1) + 1 (func_b call 2) + 1 (main) = 3
}

void tiny16_test_call_ret_preserves_registers(Tiny16CPU* cpu, Tiny16Memory* memory) {
    // Test that CALL/RET works correctly when subroutine uses PUSH/POP to preserve registers
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 10);          // R0 = 10
        /* 1 */ TINY16_ASM(TINY16_OPCODE_LOADI, 1, 20);          // R1 = 20
        /* 2 */ TINY16_ASM(TINY16_OPCODE_CALL, 0x00, ADDR16(5)); // call subroutine
        /* 3 */ TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);             // R0 += R1 (after return)
        /* 4 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);            //
        /* 5 */ TINY16_ASM(TINY16_OPCODE_PUSH, 1, 0);            // subroutine: preserve R1
        /* 6 */ TINY16_ASM(TINY16_OPCODE_LOADI, 1, 99);          // subroutine: R1 = 99 (temp)
        /* 7 */ TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);             // subroutine: R0 += 99
        /* 8 */ TINY16_ASM(TINY16_OPCODE_POP, 1, 0);             // subroutine: restore R1
        /* 9 */ TINY16_ASM(TINY16_OPCODE_RET, 0, 0);             // subroutine: return
    );
    assert(cpu->R[0] == 129); // 10 + 99 (in subroutine) + 20 (after return) = 129
    assert(cpu->R[1] == 20);  // R1 should be preserved
}
