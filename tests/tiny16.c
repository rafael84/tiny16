#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "cpu.c"
#include "cpu.h"
#include "memory.c"
#include "memory.h"
#include "ppu.c"
#include "ppu.h"
#include "vm.c"
#include "vm.h"

void tiny16_test_arithmetic(void);
void tiny16_test_borrow(void);
void tiny16_test_inc_dec_wraparound(void);
void tiny16_test_logic_ops(void);
void tiny16_test_load_store_indirect(void);
void tiny16_test_jz_conditional_jump(void);
void tiny16_test_jnz_mirror_test(void);
void tiny16_test_mov_register(void);
void tiny16_test_shift_left(void);
void tiny16_test_shift_right(void);
void tiny16_test_shift_edge_cases(void);
void tiny16_test_push_pop_basic(void);
void tiny16_test_push_pop_multiple(void);
void tiny16_test_all_registers(void);
void tiny16_test_complex_arithmetic(void);
void tiny16_test_nested_loops(void);
void tiny16_test_zero_flag(void);
void tiny16_test_carry_flag_propagation(void);
void tiny16_test_memory_boundary(void);
void tiny16_test_jump_backward_forward(void);
void tiny16_test_xor_self_zeroing(void);
void tiny16_test_address_registers_arithmetic(void);
void tiny16_test_stack_pointer_behavior(void);
void tiny16_test_conditional_jumps_not_taken(void);
void tiny16_test_logic_identity_operations(void);
void tiny16_test_chained_arithmetic_carry(void);
void tiny16_test_shift_until_zero(void);
void tiny16_test_stack_memory_access(void);
void tiny16_test_sub_equal_values(void);
void tiny16_test_max_steps_exceeded(void);
void tiny16_test_call_ret_basic(void);
void tiny16_test_call_ret_with_args(void);
void tiny16_test_call_ret_nested(void);
void tiny16_test_call_ret_preserves_registers(void);
void tiny16_test_jc_with_carry(void);
void tiny16_test_jc_without_carry(void);
void tiny16_test_jnc_with_carry(void);
void tiny16_test_jnc_without_carry(void);
void tiny16_test_cmp_equal(void);
void tiny16_test_cmp_greater(void);
void tiny16_test_cmp_less(void);
void tiny16_test_cmp_preserves_registers(void);
void tiny16_test_cmp_branching(void);
void tiny16_test_adc_no_carry(void);
void tiny16_test_adc_with_carry(void);
void tiny16_test_adc_multibyte(void);
void tiny16_test_adc_overflow(void);
void tiny16_test_sbc_no_borrow(void);
void tiny16_test_sbc_with_borrow(void);
void tiny16_test_sbc_multibyte(void);
void tiny16_test_sbc_underflow(void);

#define ADDR16(instruction) (TINY16_MEMORY_CODE_BEGIN + instruction * 3)

#define TINY16_TEST(fn)                                                                            \
    do {                                                                                           \
        printf(" ▸ %s", #fn);                                                                      \
        fflush(stdout);                                                                            \
        fn();                                                                                      \
        printf(" ✓\n");                                                                            \
    } while (0)

static Tiny16VM* vm;

#define TINY16_ASM(opcode, arg1, arg2)                                                             \
    do {                                                                                           \
        tiny16_vm_mem_write(vm, code++, opcode);                                                   \
        tiny16_vm_mem_write(vm, code++, arg1);                                                     \
        tiny16_vm_mem_write(vm, code++, arg2);                                                     \
    } while (0)

#define TINY16_TEST_RUN(...)                                                                       \
    do {                                                                                           \
        tiny16_vm_reset(vm);                                                                       \
        int code = vm->cpu.pc;                                                                     \
        __VA_ARGS__;                                                                               \
        if (!tiny16_vm_exec(vm, 20)) {                                                             \
            puts("\n- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");     \
            tiny16_cpu_print(&vm->cpu);                                                            \
            tiny16_memory_print(&vm->memory, false);                                               \
        }                                                                                          \
    } while (0)

int main(void) {
    vm = tiny16_vm_create();

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
    TINY16_TEST(tiny16_test_jc_with_carry);
    TINY16_TEST(tiny16_test_jc_without_carry);
    TINY16_TEST(tiny16_test_jnc_with_carry);
    TINY16_TEST(tiny16_test_jnc_without_carry);
    TINY16_TEST(tiny16_test_cmp_equal);
    TINY16_TEST(tiny16_test_cmp_greater);
    TINY16_TEST(tiny16_test_cmp_less);
    TINY16_TEST(tiny16_test_cmp_preserves_registers);
    TINY16_TEST(tiny16_test_cmp_branching);
    TINY16_TEST(tiny16_test_adc_no_carry);
    TINY16_TEST(tiny16_test_adc_with_carry);
    TINY16_TEST(tiny16_test_adc_multibyte);
    TINY16_TEST(tiny16_test_adc_overflow);
    TINY16_TEST(tiny16_test_sbc_no_borrow);
    TINY16_TEST(tiny16_test_sbc_with_borrow);
    TINY16_TEST(tiny16_test_sbc_multibyte);
    TINY16_TEST(tiny16_test_sbc_underflow);
    return 0;
}

void tiny16_test_arithmetic(void) {
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xFF); // R0 = 255
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x01); // R1 = 1
                    TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);      // R0 = 0, Z=1, C=1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 0);
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C));
}

void tiny16_test_borrow(void) {
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x00); // R0 = 0
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x01); // R1 = 1
                    TINY16_ASM(TINY16_OPCODE_SUB, 0, 1);      // R0 = 255, borrow
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0));
    assert(vm->cpu.R[0] == 0xFF);
    assert(!(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z)));
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C));
}

void tiny16_test_inc_dec_wraparound(void) {
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xFF); // R0 = 255
                    TINY16_ASM(TINY16_OPCODE_INC, 0, 0);      // R0 = 0, Z=1
                    TINY16_ASM(TINY16_OPCODE_DEC, 0, 0);      // R0 = 255, Z=0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0));
    assert(vm->cpu.R[0] == 0xFF);
}

void tiny16_test_logic_ops(void) {
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xF0); // 11110000
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x0F); // 00001111

                    TINY16_ASM(TINY16_OPCODE_AND, 0, 1); // R0 = 0x00, Z=1
                    TINY16_ASM(TINY16_OPCODE_OR, 0, 1);  // R0 = 0x0F
                    TINY16_ASM(TINY16_OPCODE_XOR, 0, 1); // R0 = 0x00, Z=1

                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0));
    assert(vm->cpu.R[0] == 0);
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));
}

void tiny16_test_load_store_indirect(void) {
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 6, 0x40); // R6 = 0x40
                    TINY16_ASM(TINY16_OPCODE_LOADI, 7, 0x00); // R7 = 0x00  → ADDR = 0x4000

                    TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xAB); // R0 = 0xAB
                    TINY16_ASM(TINY16_OPCODE_STORE, 0, 0x00); // MEM[0x4000] = R0

                    TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x00); // R0 = 0
                    TINY16_ASM(TINY16_OPCODE_LOAD, 0, 0x00);  // R0 = MEM[0x4000]

                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0));
    assert(vm->cpu.R[0] == 0xAB);
}

void tiny16_test_jz_conditional_jump(void) {
    TINY16_TEST_RUN(
        /* 0x00 */ TINY16_ASM(TINY16_OPCODE_LOADI, 1, 3);          // R1 = 3
        /* 0x01 */ TINY16_ASM(TINY16_OPCODE_DEC, 1, 0);            // R1--
        /* 0x02 */ TINY16_ASM(TINY16_OPCODE_JZ, 0x00, ADDR16(4));  // if Z, jump to HALT
        /* 0x03 */ TINY16_ASM(TINY16_OPCODE_JMP, 0x00, ADDR16(1)); // jump back to DEC
        /* 0x04 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);           //
    );
    assert(vm->cpu.R[1] == 0);
}

void tiny16_test_jnz_mirror_test(void) {
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 1, 3);          // R1 = 3
                    TINY16_ASM(TINY16_OPCODE_DEC, 1, 0);            // R1 = R1 - 1
                    TINY16_ASM(TINY16_OPCODE_JNZ, 0x00, ADDR16(1)); // loop while R1 != 0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);           //
    );
    assert(vm->cpu.R[1] == 0);
}

void tiny16_test_mov_register(void) {
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xAB); // R0 = 0xAB
                    TINY16_ASM(TINY16_OPCODE_MOV, 1, 0);      // R1 = R0
                    TINY16_ASM(TINY16_OPCODE_MOV, 2, 1);      // R2 = R1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 0xAB);
    assert(vm->cpu.R[1] == 0xAB);
    assert(vm->cpu.R[2] == 0xAB);
}

void tiny16_test_shift_left(void) {
    // Test SHL: shift left logical
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x01); // R0 = 0b00000001
                    TINY16_ASM(TINY16_OPCODE_SHL, 0, 0);      // R0 = 0b00000010 (2)
                    TINY16_ASM(TINY16_OPCODE_SHL, 0, 0);      // R0 = 0b00000100 (4)
                    TINY16_ASM(TINY16_OPCODE_SHL, 0, 0);      // R0 = 0b00001000 (8)
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 8);
}

void tiny16_test_shift_right(void) {
    // Test SHR: shift right logical
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x80); // R0 = 0b10000000 (128)
                    TINY16_ASM(TINY16_OPCODE_SHR, 0, 0);      // R0 = 0b01000000 (64)
                    TINY16_ASM(TINY16_OPCODE_SHR, 0, 0);      // R0 = 0b00100000 (32)
                    TINY16_ASM(TINY16_OPCODE_SHR, 0, 0);      // R0 = 0b00010000 (16)
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 16);
}

void tiny16_test_shift_edge_cases(void) {
    // Test shifting out bits and wrapping
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xFF); // R0 = 0b11111111
                    TINY16_ASM(TINY16_OPCODE_SHL, 0, 0);      // R0 = 0b11111110, carry = 1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 0xFE);
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C)); // carry should be set

    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0xFF); // R1 = 0b11111111
                    TINY16_ASM(TINY16_OPCODE_SHR, 1, 0);      // R1 = 0b01111111, carry = 1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[1] == 0x7F);
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C)); // carry should be set
}

void tiny16_test_push_pop_basic(void) {
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x42); // R0 = 0x42
                    TINY16_ASM(TINY16_OPCODE_PUSH, 0, 0);     // push R0
                    TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x00); // R0 = 0
                    TINY16_ASM(TINY16_OPCODE_POP, 0, 0);      // pop to R0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 0x42);
}

void tiny16_test_push_pop_multiple(void) {
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
    assert(vm->cpu.R[3] == 0x33);
    assert(vm->cpu.R[4] == 0x22);
    assert(vm->cpu.R[5] == 0x11);
}

void tiny16_test_all_registers(void) {
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
    assert(vm->cpu.R[0] == 0x00);
    assert(vm->cpu.R[1] == 0x11);
    assert(vm->cpu.R[2] == 0x22);
    assert(vm->cpu.R[3] == 0x33);
    assert(vm->cpu.R[4] == 0x44);
    assert(vm->cpu.R[5] == 0x55);
    assert(vm->cpu.R[6] == 0x66);
    assert(vm->cpu.R[7] == 0x77);
}

void tiny16_test_complex_arithmetic(void) {
    // Test: (10 + 20) * 2 = 60 using shifts for multiplication
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 10); // R0 = 10
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 20); // R1 = 20
                    TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);    // R0 = 30
                    TINY16_ASM(TINY16_OPCODE_SHL, 0, 0);    // R0 = 60 (multiply by 2)
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 60);
}

void tiny16_test_nested_loops(void) {
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
    assert(vm->cpu.R[0] == 4); // 2 * 2 = 4 iterations
}

void tiny16_test_zero_flag(void) {
    // Test that zero flag is set/cleared correctly
    // Z flag is only set by ALU operations, not LOADI
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x05); // R0 = 5
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x05); // R1 = 5
                    TINY16_ASM(TINY16_OPCODE_SUB, 0, 1);      // R0 = 0, Z should be set
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 0);
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));

    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x01); // R0 = 1
                    TINY16_ASM(TINY16_OPCODE_DEC, 0, 0);      // R0 = 0, Z should be set
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 0);
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));

    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x05); // R0 = 5
                    TINY16_ASM(TINY16_OPCODE_INC, 0, 0);      // R0 = 6, Z should be clear
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 6);
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));
}

void tiny16_test_carry_flag_propagation(void) {
    // Test carry flag in various operations
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x80); // R0 = 128
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x80); // R1 = 128
                    TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);      // R0 = 0, C = 1 (overflow)
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 0);
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C));
}

void tiny16_test_memory_boundary(void) {
    // Test loading and storing at various memory locations in data segment
    TINY16_TEST_RUN(
        TINY16_ASM(TINY16_OPCODE_LOADI, 6, 0x40); // R6:R7 = 0x4000 (data segment start)
        TINY16_ASM(TINY16_OPCODE_LOADI, 7, 0x00); //
        TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xAA); // R0 = 0xAA
        TINY16_ASM(TINY16_OPCODE_STORE, 0, 0);    // MEM[0x4000] = 0xAA

        TINY16_ASM(TINY16_OPCODE_LOADI, 6, 0x48); // R6:R7 = 0x4800 (middle of data segment)
        TINY16_ASM(TINY16_OPCODE_LOADI, 7, 0x00); //
        TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0xBB); // R1 = 0xBB
        TINY16_ASM(TINY16_OPCODE_STORE, 1, 0);    // MEM[0x4800] = 0xBB

        TINY16_ASM(TINY16_OPCODE_LOADI, 6, 0x40); // R6:R7 = 0x4000
        TINY16_ASM(TINY16_OPCODE_LOADI, 7, 0x00); //
        TINY16_ASM(TINY16_OPCODE_LOAD, 2, 0);     // R2 = MEM[0x4000]

        TINY16_ASM(TINY16_OPCODE_LOADI, 6, 0x48); // R6:R7 = 0x4800
        TINY16_ASM(TINY16_OPCODE_LOADI, 7, 0x00); //
        TINY16_ASM(TINY16_OPCODE_LOAD, 3, 0);     // R3 = MEM[0x4800]

        TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[2] == 0xAA);
    assert(vm->cpu.R[3] == 0xBB);
}

void tiny16_test_jump_backward_forward(void) {
    // Test forward and backward jumps
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0);          // R0 = 0
        /* 1 */ TINY16_ASM(TINY16_OPCODE_JMP, 0x00, ADDR16(3)); // jump forward to instruction 3
        /* 2 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 99);         // this should be skipped
        /* 3 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 42);         // R0 = 42
        /* 4 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);           //
    );
    assert(vm->cpu.R[0] == 42); // Should be 42, not 0 or 99
}

void tiny16_test_xor_self_zeroing(void) {
    // Test XOR R, R is a common idiom to zero a register efficiently
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xAB); // R0 = 0xAB
                    TINY16_ASM(TINY16_OPCODE_XOR, 0, 0);      // R0 = R0 XOR R0 = 0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 0);
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));
}

void tiny16_test_address_registers_arithmetic(void) {
    // Test that R6 and R7 can be used for arithmetic, not just addressing
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 6, 10); // R6 = 10
                    TINY16_ASM(TINY16_OPCODE_LOADI, 7, 5);  // R7 = 5
                    TINY16_ASM(TINY16_OPCODE_ADD, 6, 7);    // R6 = R6 + R7 = 15
                    TINY16_ASM(TINY16_OPCODE_SUB, 7, 7);    // R7 = R7 - R7 = 0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[6] == 15);
    assert(vm->cpu.R[7] == 0);
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));
}

void tiny16_test_stack_pointer_behavior(void) {
    // Verify stack pointer changes correctly with PUSH and POP
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x42); // R0 = 0x42
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    uint16_t initial_sp = vm->cpu.sp;

    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x11); // R0 = 0x11
                    TINY16_ASM(TINY16_OPCODE_PUSH, 0, 0);     // push R0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.sp == initial_sp - 1); // SP should decrement

    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x11); // R0 = 0x11
                    TINY16_ASM(TINY16_OPCODE_PUSH, 0, 0);     // push R0
                    TINY16_ASM(TINY16_OPCODE_POP, 1, 0);      // pop to R1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.sp == initial_sp); // SP should be back to initial
}

void tiny16_test_conditional_jumps_not_taken(void) {
    // Test that conditional jumps fall through when condition is false
    // JZ should NOT jump when Z=0
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 1);         // R0 = 1
        /* 1 */ TINY16_ASM(TINY16_OPCODE_INC, 0, 0);           // R0 = 2, Z=0
        /* 2 */ TINY16_ASM(TINY16_OPCODE_JZ, 0x00, ADDR16(4)); // should NOT jump (Z=0)
        /* 3 */ TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x42);      // R1 = 0x42 (executed)
        /* 4 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);          //
    );
    assert(vm->cpu.R[1] == 0x42); // Should have executed line 3

    // JNZ should NOT jump when Z=1
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 1);          // R0 = 1
        /* 1 */ TINY16_ASM(TINY16_OPCODE_DEC, 0, 0);            // R0 = 0, Z=1
        /* 2 */ TINY16_ASM(TINY16_OPCODE_JNZ, 0x00, ADDR16(4)); // should NOT jump (Z=1)
        /* 3 */ TINY16_ASM(TINY16_OPCODE_LOADI, 2, 0x99);       // R2 = 0x99 (executed)
        /* 4 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);           //
    );
    assert(vm->cpu.R[2] == 0x99); // Should have executed line 3
}

void tiny16_test_logic_identity_operations(void) {
    // Test logic operations that should preserve or set specific values
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x55); // R0 = 0x55
                    TINY16_ASM(TINY16_OPCODE_OR, 0, 0);       // R0 OR R0 = R0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 0x55);

    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0xAA); // R1 = 0xAA
                    TINY16_ASM(TINY16_OPCODE_AND, 1, 1);      // R1 AND R1 = R1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[1] == 0xAA);

    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 2, 0xFF); // R2 = 0xFF
                    TINY16_ASM(TINY16_OPCODE_LOADI, 3, 0x00); // R3 = 0x00
                    TINY16_ASM(TINY16_OPCODE_AND, 2, 3);      // R2 AND 0 = 0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[2] == 0);
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));

    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 4, 0x00); // R4 = 0x00
                    TINY16_ASM(TINY16_OPCODE_LOADI, 5, 0xFF); // R5 = 0xFF
                    TINY16_ASM(TINY16_OPCODE_OR, 4, 5);       // R4 OR 0xFF = 0xFF
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[4] == 0xFF);
}

void tiny16_test_chained_arithmetic_carry(void) {
    // Test multiple arithmetic operations and carry flag behavior
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xFF); // R0 = 255
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x01); // R1 = 1
                    TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);      // R0 = 0, C=1
                    TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);      // R0 = 1, C=0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 1);
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C));
}

void tiny16_test_shift_until_zero(void) {
    // Test shifting a value repeatedly until it becomes zero
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x08); // R0 = 8 (0b00001000)
                    TINY16_ASM(TINY16_OPCODE_SHR, 0, 0);      // R0 = 4
                    TINY16_ASM(TINY16_OPCODE_SHR, 0, 0);      // R0 = 2
                    TINY16_ASM(TINY16_OPCODE_SHR, 0, 0);      // R0 = 1
                    TINY16_ASM(TINY16_OPCODE_SHR, 0, 0);      // R0 = 0, Z=1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 0);
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));
}

void tiny16_test_stack_memory_access(void) {
    // Test loading and storing at stack segment addresses
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 6, 0x80); // R6:R7 = 0x8000 (stack begin)
                    TINY16_ASM(TINY16_OPCODE_LOADI, 7, 0x00); //
                    TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0xCD); // R0 = 0xCD
                    TINY16_ASM(TINY16_OPCODE_STORE, 0, 0);    // MEM[0x8000] = 0xCD

                    TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x00); // R0 = 0
                    TINY16_ASM(TINY16_OPCODE_LOAD, 0, 0);     // R0 = MEM[0x8000]

                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 0xCD);
}

void tiny16_test_sub_equal_values(void) {
    // Test SUB with equal values: should give 0, set Z, clear C
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x42); // R0 = 0x42
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x42); // R1 = 0x42
                    TINY16_ASM(TINY16_OPCODE_SUB, 0, 1);      // R0 = 0, Z=1, C=0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 0);
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C));
}

void tiny16_test_max_steps_exceeded(void) {
    tiny16_vm_reset(vm);
    int code = vm->cpu.pc;
    TINY16_ASM(TINY16_OPCODE_JMP, 0x00, ADDR16(0)); // infinite loop: jump to itself
    bool result = tiny16_vm_exec(vm, 5);
    assert(result);
    assert(vm->cpu.pc == TINY16_MEMORY_CODE_BEGIN);
}

void tiny16_test_call_ret_basic(void) {
    // Test basic CALL/RET: call a subroutine that increments R0, then return
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 10);          // R0 = 10
        /* 1 */ TINY16_ASM(TINY16_OPCODE_CALL, 0x00, ADDR16(4)); // call subroutine at 4
        /* 2 */ TINY16_ASM(TINY16_OPCODE_INC, 0, 0);             // R0++ (after return)
        /* 3 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);            //
        /* 4 */ TINY16_ASM(TINY16_OPCODE_INC, 0, 0);             // subroutine: R0++
        /* 5 */ TINY16_ASM(TINY16_OPCODE_RET, 0, 0);             // return
    );
    assert(vm->cpu.R[0] == 12); // 10 + 1 (in subroutine) + 1 (after return)
}

void tiny16_test_call_ret_with_args(void) {
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
    assert(vm->cpu.R[0] == 13); // 5 + 7 + 1 = 13
}

void tiny16_test_call_ret_nested(void) {
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
    assert(vm->cpu.R[0] == 3); // 0 + 1 (func_b call 1) + 1 (func_b call 2) + 1 (main) = 3
}

void tiny16_test_call_ret_preserves_registers(void) {
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
    assert(vm->cpu.R[0] == 129); // 10 + 99 (in subroutine) + 20 (after return) = 129
    assert(vm->cpu.R[1] == 20);  // R1 should be preserved
}

void tiny16_test_jc_with_carry(void) {
    // Test JC jumps when carry is set (ADD overflow)
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 200);       // R0 = 200
        /* 1 */ TINY16_ASM(TINY16_OPCODE_LOADI, 1, 100);       // R1 = 100
        /* 2 */ TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);           // R0 = 44, C = 1 (overflow)
        /* 3 */ TINY16_ASM(TINY16_OPCODE_JC, 0x00, ADDR16(5)); // should jump (C = 1)
        /* 4 */ TINY16_ASM(TINY16_OPCODE_LOADI, 2, 99);        // should NOT execute
        /* 5 */ TINY16_ASM(TINY16_OPCODE_LOADI, 2, 42);        // R2 = 42 (after jump)
        /* 6 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);          //
    );
    assert(vm->cpu.R[0] == 44); // 200 + 100 = 300 & 0xFF = 44
    assert(vm->cpu.R[2] == 42); // Should have jumped, so R2 = 42, not 99
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C));
}

void tiny16_test_jc_without_carry(void) {
    // Test JC does not jump when carry is clear
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 50);         // R0 = 50
        /* 1 */ TINY16_ASM(TINY16_OPCODE_LOADI, 1, 30);         // R1 = 30
        /* 2 */ TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);            // R0 = 80, C = 0 (no overflow)
        /* 3 */ TINY16_ASM(TINY16_OPCODE_JC, 0x00, ADDR16(6));  // should NOT jump (C = 0)
        /* 4 */ TINY16_ASM(TINY16_OPCODE_LOADI, 2, 99);         // should execute
        /* 5 */ TINY16_ASM(TINY16_OPCODE_JMP, 0x00, ADDR16(7)); // skip over error code
        /* 6 */ TINY16_ASM(TINY16_OPCODE_LOADI, 2, 42);         // error: should NOT execute
        /* 7 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);           //
    );
    assert(vm->cpu.R[0] == 80); // 50 + 30 = 80
    assert(vm->cpu.R[2] == 99); // Should NOT have jumped, so R2 = 99
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C));
}

void tiny16_test_jnc_with_carry(void) {
    // Test JNC does not jump when carry is set
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 200);        // R0 = 200
        /* 1 */ TINY16_ASM(TINY16_OPCODE_LOADI, 1, 100);        // R1 = 100
        /* 2 */ TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);            // R0 = 44, C = 1 (overflow)
        /* 3 */ TINY16_ASM(TINY16_OPCODE_JNC, 0x00, ADDR16(6)); // should NOT jump (C = 1)
        /* 4 */ TINY16_ASM(TINY16_OPCODE_LOADI, 2, 99);         // should execute
        /* 5 */ TINY16_ASM(TINY16_OPCODE_JMP, 0x00, ADDR16(7)); // skip over error code
        /* 6 */ TINY16_ASM(TINY16_OPCODE_LOADI, 2, 42);         // error: should NOT execute
        /* 7 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);           //
    );
    assert(vm->cpu.R[0] == 44); // 200 + 100 = 300 & 0xFF = 44
    assert(vm->cpu.R[2] == 99); // Should NOT have jumped, so R2 = 99
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C));
}

void tiny16_test_jnc_without_carry(void) {
    // Test JNC jumps when carry is clear
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 50);         // R0 = 50
        /* 1 */ TINY16_ASM(TINY16_OPCODE_LOADI, 1, 30);         // R1 = 30
        /* 2 */ TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);            // R0 = 80, C = 0 (no overflow)
        /* 3 */ TINY16_ASM(TINY16_OPCODE_JNC, 0x00, ADDR16(5)); // should jump (C = 0)
        /* 4 */ TINY16_ASM(TINY16_OPCODE_LOADI, 2, 99);         // should NOT execute
        /* 5 */ TINY16_ASM(TINY16_OPCODE_LOADI, 2, 42);         // R2 = 42 (after jump)
        /* 6 */ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);           //
    );
    assert(vm->cpu.R[0] == 80); // 50 + 30 = 80
    assert(vm->cpu.R[2] == 42); // Should have jumped, so R2 = 42, not 99
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C));
}

void tiny16_test_cmp_equal(void) {
    // Test CMP with equal values: should set Z=1, C=0
    // CMP does not modify the registers
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 42); // R0 = 42
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 42); // R1 = 42
                    TINY16_ASM(TINY16_OPCODE_CMP, 0, 1);    // compare R0 - R1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 42);                                 // R0 should be unchanged
    assert(vm->cpu.R[1] == 42);                                 // R1 should be unchanged
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));  // Z = 1 (equal)
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C)); // C = 0 (no borrow)
}

void tiny16_test_cmp_greater(void) {
    // Test CMP when first operand is greater: should set Z=0, C=0
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 100); // R0 = 100
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 50);  // R1 = 50
                    TINY16_ASM(TINY16_OPCODE_CMP, 0, 1);     // compare R0 - R1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 100);                                // R0 should be unchanged
    assert(vm->cpu.R[1] == 50);                                 // R1 should be unchanged
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z)); // Z = 0 (not equal)
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C)); // C = 0 (no borrow)
}

void tiny16_test_cmp_less(void) {
    // Test CMP when first operand is less: should set Z=0, C=1
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 30);  // R0 = 30
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 100); // R1 = 100
                    TINY16_ASM(TINY16_OPCODE_CMP, 0, 1);     // compare R0 - R1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 30);                                 // R0 should be unchanged
    assert(vm->cpu.R[1] == 100);                                // R1 should be unchanged
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z)); // Z = 0 (not equal)
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C));  // C = 1 (borrow occurred)
}

void tiny16_test_cmp_preserves_registers(void) {
    // Test that CMP preserves all registers including edge cases
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0);   // R0 = 0
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 255); // R1 = 255
                    TINY16_ASM(TINY16_OPCODE_LOADI, 2, 128); // R2 = 128
                    TINY16_ASM(TINY16_OPCODE_CMP, 0, 1);     // compare R0 - R1
                    TINY16_ASM(TINY16_OPCODE_CMP, 1, 2);     // compare R1 - R2
                    TINY16_ASM(TINY16_OPCODE_CMP, 2, 0);     // compare R2 - R0
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 0);   // R0 unchanged
    assert(vm->cpu.R[1] == 255); // R1 unchanged
    assert(vm->cpu.R[2] == 128); // R2 unchanged
}

void tiny16_test_cmp_branching(void) {
    // Test CMP with conditional branching (like example 14_cmp.asm)
    // Compare R0=12 with R1=10, should branch to "greater"
    TINY16_TEST_RUN(
        /* 0 */ TINY16_ASM(TINY16_OPCODE_LOADI, 0, 12);          // R0 = 12
        /* 1 */ TINY16_ASM(TINY16_OPCODE_LOADI, 1, 10);          // R1 = 10
        /* 2 */ TINY16_ASM(TINY16_OPCODE_CMP, 0, 1);             // compare R0 - R1
        /* 3 */ TINY16_ASM(TINY16_OPCODE_JZ, 0x00, ADDR16(9));   // if Z=1 (equal), jump to equal
        /* 4 */ TINY16_ASM(TINY16_OPCODE_JC, 0x00, ADDR16(11));  // if C=1 (less), jump to less
        /* 5 */ TINY16_ASM(TINY16_OPCODE_LOADI, 2, 100);         // greater: R2 = 100
        /* 6 */ TINY16_ASM(TINY16_OPCODE_JMP, 0x00, ADDR16(13)); // jump to end
        /* 7 */ TINY16_ASM(TINY16_OPCODE_LOADI, 2, 50); // equal: R2 = 50 (should NOT execute)
        /* 8 */ TINY16_ASM(TINY16_OPCODE_JMP, 0x00, ADDR16(13)); // jump to end
        /* 9 */ TINY16_ASM(TINY16_OPCODE_LOADI, 2, 50); // equal: R2 = 50 (should NOT execute)
        /* 10*/ TINY16_ASM(TINY16_OPCODE_JMP, 0x00, ADDR16(13)); // jump to end
        /* 11*/ TINY16_ASM(TINY16_OPCODE_LOADI, 2, 0); // less: R2 = 0 (should NOT execute)
        /* 12*/ TINY16_ASM(TINY16_OPCODE_JMP, 0x00, ADDR16(13)); // jump to end
        /* 13*/ TINY16_ASM(TINY16_OPCODE_HALT, 0, 0);            // end
    );
    assert(vm->cpu.R[0] == 12);  // R0 unchanged
    assert(vm->cpu.R[1] == 10);  // R1 unchanged
    assert(vm->cpu.R[2] == 100); // Took the "greater" path
}

void tiny16_test_adc_no_carry(void) {
    // Test ADC with carry flag clear (C=0)
    // ADC should behave like ADD when C=0
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 10); // R0 = 10
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 20); // R1 = 20
                    TINY16_ASM(TINY16_OPCODE_ADC, 0, 1);    // R0 = 10 + 20 + 0 = 30
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 30);
    assert(vm->cpu.R[1] == 20); // R1 unchanged
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C));
}

void tiny16_test_adc_with_carry(void) {
    // Test ADC with carry flag set (C=1)
    // First ADD sets carry, then ADC uses it
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 200); // R0 = 200
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 100); // R1 = 100
                    TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);     // R0 = 44 (overflow), C=1
                    TINY16_ASM(TINY16_OPCODE_LOADI, 2, 10);  // R2 = 10
                    TINY16_ASM(TINY16_OPCODE_LOADI, 3, 20);  // R3 = 20
                    TINY16_ASM(TINY16_OPCODE_ADC, 2, 3);     // R2 = 10 + 20 + 1 = 31
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 44); // 200 + 100 = 300 & 0xFF = 44
    assert(vm->cpu.R[2] == 31); // 10 + 20 + 1 (carry) = 31
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C)); // No carry from last ADC
}

void tiny16_test_adc_multibyte(void) {
    // Test 16-bit addition using ADD + ADC
    // Add R6:R7 = 0x01FF (511) + R0:R1 = 0x0002 (2) = 0x0201 (513)
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 6, 0x01); // R6 = 0x01 (high byte)
                    TINY16_ASM(TINY16_OPCODE_LOADI, 7, 0xFF); // R7 = 0xFF (low byte)
                    TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x00); // R0 = 0x00 (high byte)
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0x02); // R1 = 0x02 (low byte)
                    TINY16_ASM(TINY16_OPCODE_ADD, 7, 1);      // R7 = 0xFF + 0x02 = 0x01, C=1
                    TINY16_ASM(TINY16_OPCODE_ADC, 6, 0);      // R6 = 0x01 + 0x00 + 1 = 0x02
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[6] == 0x02); // High byte result
    assert(vm->cpu.R[7] == 0x01); // Low byte result
    // R6:R7 = 0x0201 (513)
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C));
}

void tiny16_test_adc_overflow(void) {
    // Test ADC with overflow (sets carry flag)
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 250); // R0 = 250
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 10);  // R1 = 10
                    TINY16_ASM(TINY16_OPCODE_ADD, 0, 1);     // R0 = 4, C=1 (overflow)
                    TINY16_ASM(TINY16_OPCODE_LOADI, 2, 250); // R2 = 250
                    TINY16_ASM(TINY16_OPCODE_LOADI, 3, 5);   // R3 = 5
                    TINY16_ASM(TINY16_OPCODE_ADC, 2, 3);     // R2 = 250 + 5 + 1 = 0, C=1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[2] == 0);                                 // 250 + 5 + 1 = 256 & 0xFF = 0
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z)); // Result is zero
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C)); // Overflow occurred
}

void tiny16_test_sbc_no_borrow(void) {
    // Test SBC with carry flag clear (C=0, no borrow)
    // SBC should behave like SUB when C=0
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 50); // R0 = 50
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 20); // R1 = 20
                    TINY16_ASM(TINY16_OPCODE_SBC, 0, 1);    // R0 = 50 - 20 - 0 = 30
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 30);
    assert(vm->cpu.R[1] == 20); // R1 unchanged
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C));
}

void tiny16_test_sbc_with_borrow(void) {
    // Test SBC with carry flag set (C=1, borrow)
    // First SUB sets carry (borrow), then SBC uses it
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 5);  // R0 = 5
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 10); // R1 = 10
                    TINY16_ASM(TINY16_OPCODE_SUB, 0, 1);    // R0 = 251, C=1 (borrow)
                    TINY16_ASM(TINY16_OPCODE_LOADI, 2, 50); // R2 = 50
                    TINY16_ASM(TINY16_OPCODE_LOADI, 3, 20); // R3 = 20
                    TINY16_ASM(TINY16_OPCODE_SBC, 2, 3);    // R2 = 50 - 20 - 1 = 29
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[0] == 251); // 5 - 10 = -5 & 0xFF = 251
    assert(vm->cpu.R[2] == 29);  // 50 - 20 - 1 (borrow) = 29
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C)); // No borrow from last SBC
}

void tiny16_test_sbc_multibyte(void) {
    // Test 16-bit subtraction using SUB + SBC
    // Subtract R6:R7 = 0x0201 (513) - R0:R1 = 0x00FF (255) = 0x0102 (258)
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 6, 0x02); // R6 = 0x02 (high byte)
                    TINY16_ASM(TINY16_OPCODE_LOADI, 7, 0x01); // R7 = 0x01 (low byte)
                    TINY16_ASM(TINY16_OPCODE_LOADI, 0, 0x00); // R0 = 0x00 (high byte)
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 0xFF); // R1 = 0xFF (low byte)
                    TINY16_ASM(TINY16_OPCODE_SUB, 7, 1);      // R7 = 0x01 - 0xFF = 0x02, C=1
                    TINY16_ASM(TINY16_OPCODE_SBC, 6, 0);      // R6 = 0x02 - 0x00 - 1 = 0x01
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[6] == 0x01); // High byte result
    assert(vm->cpu.R[7] == 0x02); // Low byte result
    // R6:R7 = 0x0102 (258)
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C));
}

void tiny16_test_sbc_underflow(void) {
    // Test SBC with underflow (sets carry flag)
    TINY16_TEST_RUN(TINY16_ASM(TINY16_OPCODE_LOADI, 0, 10); // R0 = 10
                    TINY16_ASM(TINY16_OPCODE_LOADI, 1, 5);  // R1 = 5
                    TINY16_ASM(TINY16_OPCODE_SUB, 0, 1);    // R0 = 5, C=0 (no borrow)
                    TINY16_ASM(TINY16_OPCODE_LOADI, 2, 5);  // R2 = 5
                    TINY16_ASM(TINY16_OPCODE_LOADI, 3, 10); // R3 = 10
                    TINY16_ASM(TINY16_OPCODE_SBC, 2, 3);    // R2 = 5 - 10 - 0 = 251, C=1
                    TINY16_ASM(TINY16_OPCODE_HALT, 0, 0););
    assert(vm->cpu.R[2] == 251); // 5 - 10 = -5 & 0xFF = 251
    assert(!TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_Z));
    assert(TINY16_CPU_FLAG(vm->cpu.flags, TINY16_CPU_FLAG_C)); // Underflow (borrow) occurred
}
