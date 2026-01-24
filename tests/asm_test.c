#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../asm/lexer.c"
#include "../asm/lexer.h"
#include "../asm/parser.c"
#include "../asm/parser.h"
#include "cpu.c"
#include "cpu.h"
#include "memory.c"
#include "memory.h"

#define ASM_TEST(fn)                                                                               \
    do {                                                                                           \
        printf(" ▸ %s", #fn);                                                                      \
        fflush(stdout);                                                                            \
        fn();                                                                                      \
        printf(" ✓\n");                                                                            \
    } while (0)

void test_lexer_empty(void);
void test_lexer_comments(void);
void test_lexer_symbols(void);
void test_lexer_numbers_decimal(void);
void test_lexer_numbers_hex(void);
void test_lexer_numbers_binary(void);
void test_lexer_strings(void);
void test_lexer_punctuation(void);
void test_lexer_keywords(void);
void test_lexer_multiline(void);
void test_lexer_whitespace(void);
void test_parser_label_declaration(void);
void test_parser_label_resolution(void);
void test_parser_duplicate_label(void);
void test_parser_section_code(void);
void test_parser_section_data(void);
void test_parser_instruction_loadi(void);
void test_parser_instruction_arithmetic(void);
void test_parser_instruction_jumps(void);
void test_parser_instruction_load_store(void);
void test_parser_register_parsing(void);
void test_parser_register_pairs(void);
void test_parser_addressing_modes(void);
void test_parser_data_directive_db(void);
void test_parser_data_directive_string(void);
void test_parser_data_directive_times(void);
void test_parser_data_directive_org_pads_forward(void);
void test_parser_data_directive_org_rewind_errors(void);
void test_parser_data_directive_org_out_of_range_errors(void);
void test_parser_data_directive_org_updates_label_addr(void);
void test_parser_const_simple_number(void);
void test_parser_const_duplicate_errors(void);
void test_parser_const_label_collision_errors(void);
void test_parser_const_out_of_range_errors(void);
void test_parser_complete_program(void);
void test_parser_label_forward_reference(void);
void test_parser_label_backward_reference(void);
void test_integration_simple_program(void);
void test_integration_loop_program(void);
void test_integration_subroutine_program(void);
void test_parser_expr_simple_number(void);
void test_parser_expr_hex_binary_numbers(void);
void test_parser_expr_symbol_reference(void);
void test_parser_expr_addition_subtraction(void);
void test_parser_expr_multiplication_division_modulo(void);
void test_parser_expr_bitwise_operators(void);
void test_parser_expr_shift_operators(void);
void test_parser_expr_unary_minus(void);
void test_parser_expr_unary_not(void);
void test_parser_expr_parentheses(void);
void test_parser_expr_precedence(void);
void test_parser_expr_complex(void);
void test_parser_expr_division_by_zero(void);
void test_parser_expr_undefined_symbol(void);

int main(void) {
    ASM_TEST(test_lexer_empty);
    ASM_TEST(test_lexer_comments);
    ASM_TEST(test_lexer_symbols);
    ASM_TEST(test_lexer_numbers_decimal);
    ASM_TEST(test_lexer_numbers_hex);
    ASM_TEST(test_lexer_numbers_binary);
    ASM_TEST(test_lexer_strings);
    ASM_TEST(test_lexer_punctuation);
    ASM_TEST(test_lexer_keywords);
    ASM_TEST(test_lexer_multiline);
    ASM_TEST(test_lexer_whitespace);
    ASM_TEST(test_parser_label_declaration);
    ASM_TEST(test_parser_label_resolution);
    ASM_TEST(test_parser_duplicate_label);
    ASM_TEST(test_parser_section_code);
    ASM_TEST(test_parser_section_data);
    ASM_TEST(test_parser_instruction_loadi);
    ASM_TEST(test_parser_instruction_arithmetic);
    ASM_TEST(test_parser_instruction_jumps);
    ASM_TEST(test_parser_instruction_load_store);
    ASM_TEST(test_parser_register_parsing);
    ASM_TEST(test_parser_register_pairs);
    ASM_TEST(test_parser_addressing_modes);
    ASM_TEST(test_parser_data_directive_db);
    ASM_TEST(test_parser_data_directive_string);
    ASM_TEST(test_parser_data_directive_times);
    ASM_TEST(test_parser_data_directive_org_pads_forward);
    ASM_TEST(test_parser_data_directive_org_rewind_errors);
    ASM_TEST(test_parser_data_directive_org_out_of_range_errors);
    ASM_TEST(test_parser_data_directive_org_updates_label_addr);
    ASM_TEST(test_parser_const_simple_number);
    ASM_TEST(test_parser_const_duplicate_errors);
    ASM_TEST(test_parser_const_label_collision_errors);
    ASM_TEST(test_parser_const_out_of_range_errors);
    ASM_TEST(test_parser_label_forward_reference);
    ASM_TEST(test_parser_label_backward_reference);
    ASM_TEST(test_parser_expr_simple_number);
    ASM_TEST(test_parser_expr_hex_binary_numbers);
    ASM_TEST(test_parser_expr_symbol_reference);
    ASM_TEST(test_parser_expr_addition_subtraction);
    ASM_TEST(test_parser_expr_multiplication_division_modulo);
    ASM_TEST(test_parser_expr_bitwise_operators);
    ASM_TEST(test_parser_expr_shift_operators);
    ASM_TEST(test_parser_expr_unary_minus);
    ASM_TEST(test_parser_expr_unary_not);
    ASM_TEST(test_parser_expr_parentheses);
    ASM_TEST(test_parser_expr_precedence);
    ASM_TEST(test_parser_expr_complex);
    ASM_TEST(test_parser_expr_division_by_zero);
    ASM_TEST(test_parser_expr_undefined_symbol);
    ASM_TEST(test_integration_simple_program);
    ASM_TEST(test_integration_loop_program);
    return 0;
}

void test_lexer_empty(void) {
    const char* input = "";
    Lexer lexer = lexer_new(input, strlen(input));
    Token token = lexer_next(&lexer);
    assert(token.kind == TOKEN_END);
    assert(token.line == 1);
}

void test_lexer_comments(void) {
    const char* input = "; this is a comment\n";
    Lexer lexer = lexer_new(input, strlen(input));
    Token token = lexer_next(&lexer);
    assert(token.kind == TOKEN_COMMENT);
    assert(token.line == 1);

    token = lexer_next(&lexer);
    assert(token.kind == TOKEN_EOL);

    token = lexer_next(&lexer);
    assert(token.kind == TOKEN_END);
}

void test_lexer_symbols(void) {
    const char* input = "loadi add r0 label123";
    Lexer lexer = lexer_new(input, strlen(input));

    Token token = lexer_next(&lexer);
    assert(token.kind == TOKEN_SYMBOL);
    assert(strncmp(token.text, "loadi", token.text_len) == 0);

    token = lexer_next(&lexer);
    assert(token.kind == TOKEN_SYMBOL);
    assert(strncmp(token.text, "add", token.text_len) == 0);

    token = lexer_next(&lexer);
    assert(token.kind == TOKEN_SYMBOL);
    assert(strncmp(token.text, "r0", token.text_len) == 0);

    token = lexer_next(&lexer);
    assert(token.kind == TOKEN_SYMBOL);
    assert(strncmp(token.text, "label123", token.text_len) == 0);
}

void test_lexer_numbers_decimal(void) {
    const char* input = "0 42 255 1234";
    Lexer lexer = lexer_new(input, strlen(input));

    Token token = lexer_next(&lexer);
    assert(token.kind == TOKEN_NUMBER);
    assert(strncmp(token.text, "0", token.text_len) == 0);

    token = lexer_next(&lexer);
    assert(token.kind == TOKEN_NUMBER);
    assert(strncmp(token.text, "42", token.text_len) == 0);

    token = lexer_next(&lexer);
    assert(token.kind == TOKEN_NUMBER);
    assert(strncmp(token.text, "255", token.text_len) == 0);

    token = lexer_next(&lexer);
    assert(token.kind == TOKEN_NUMBER);
    assert(strncmp(token.text, "1234", token.text_len) == 0);
}

void test_lexer_numbers_hex(void) {
    const char* input = "0x00 0xFF 0xABCD 0x1234";
    Lexer lexer = lexer_new(input, strlen(input));

    Token token = lexer_next(&lexer);
    assert(token.kind == TOKEN_NUMBER);
    assert(strncmp(token.text, "0x00", token.text_len) == 0);

    token = lexer_next(&lexer);
    assert(token.kind == TOKEN_NUMBER);
    assert(strncmp(token.text, "0xFF", token.text_len) == 0);

    token = lexer_next(&lexer);
    assert(token.kind == TOKEN_NUMBER);
    assert(strncmp(token.text, "0xABCD", token.text_len) == 0);
}

void test_lexer_numbers_binary(void) {
    const char* input = "0b0 0b1010 0b11111111";
    Lexer lexer = lexer_new(input, strlen(input));

    Token token = lexer_next(&lexer);
    assert(token.kind == TOKEN_NUMBER);
    assert(strncmp(token.text, "0b0", token.text_len) == 0);

    token = lexer_next(&lexer);
    assert(token.kind == TOKEN_NUMBER);
    assert(strncmp(token.text, "0b1010", token.text_len) == 0);

    token = lexer_next(&lexer);
    assert(token.kind == TOKEN_NUMBER);
    assert(strncmp(token.text, "0b11111111", token.text_len) == 0);
}

void test_lexer_strings(void) {
    const char* input = "\"hello\" \"world\"";
    Lexer lexer = lexer_new(input, strlen(input));

    Token token = lexer_next(&lexer);
    assert(token.kind == TOKEN_STRING);
    // String tokens include the quotes
    assert(strncmp(token.text, "\"hello\"", token.text_len) == 0);

    token = lexer_next(&lexer);
    assert(token.kind == TOKEN_STRING);
    assert(strncmp(token.text, "\"world\"", token.text_len) == 0);
}

void test_lexer_punctuation(void) {
    const char* input = ", : [ ] + -";
    Lexer lexer = lexer_new(input, strlen(input));

    assert(lexer_next(&lexer).kind == TOKEN_COMMA);
    assert(lexer_next(&lexer).kind == TOKEN_COLON);
    assert(lexer_next(&lexer).kind == TOKEN_LBRACKET);
    assert(lexer_next(&lexer).kind == TOKEN_RBRACKET);
    assert(lexer_next(&lexer).kind == TOKEN_PLUS);
    assert(lexer_next(&lexer).kind == TOKEN_MINUS);
}

void test_lexer_keywords(void) {
    const char* input = "section org times db SECTION ORG TIMES DB";
    Lexer lexer = lexer_new(input, strlen(input));

    // Keywords are case-insensitive
    assert(lexer_next(&lexer).kind == TOKEN_KEYWORD);
    assert(lexer_next(&lexer).kind == TOKEN_KEYWORD);
    assert(lexer_next(&lexer).kind == TOKEN_KEYWORD);
    assert(lexer_next(&lexer).kind == TOKEN_KEYWORD);
    assert(lexer_next(&lexer).kind == TOKEN_KEYWORD);
    assert(lexer_next(&lexer).kind == TOKEN_KEYWORD);
    assert(lexer_next(&lexer).kind == TOKEN_KEYWORD);
    assert(lexer_next(&lexer).kind == TOKEN_KEYWORD);
}

void test_lexer_multiline(void) {
    const char* input = "loadi r0, 10\nadd r0, r1\n";
    Lexer lexer = lexer_new(input, strlen(input));

    Token token = lexer_next(&lexer);
    assert(token.kind == TOKEN_SYMBOL);
    assert(token.line == 1);

    while (token.kind != TOKEN_EOL) {
        token = lexer_next(&lexer);
    }
    assert(token.line == 1);

    token = lexer_next(&lexer);
    assert(token.kind == TOKEN_SYMBOL);
    assert(token.line == 2);
}

void test_lexer_whitespace(void) {
    const char* input = "  loadi  \t  r0  ";
    Lexer lexer = lexer_new(input, strlen(input));

    Token token = lexer_next(&lexer);
    assert(token.kind == TOKEN_SYMBOL);
    assert(strncmp(token.text, "loadi", token.text_len) == 0);

    token = lexer_next(&lexer);
    assert(token.kind == TOKEN_SYMBOL);
    assert(strncmp(token.text, "r0", token.text_len) == 0);
}

void test_parser_label_declaration(void) {
    const char* input = "start:\n";
    Lexer lexer = lexer_new(input, strlen(input));
    Tiny16Parser parser = {0};
    parser.lexer = lexer;
    parser.source_filename = "test";
    parser.current_section = TINY16_PARSER_SECTION_CODE;
    parser.code_pc = TINY16_MEMORY_CODE_BEGIN;

    tiny16_parser_next(&parser);
    bool result = tiny16_parser_parse_label(&parser);

    assert(result == true);
    assert(parser.symbol_count == 1);
    assert(strcmp(parser.symbols[0].name, "start") == 0);
    assert(parser.symbols[0].kind == TINY16_SYMBOL_LABEL);
    assert(parser.symbols[0].value == TINY16_MEMORY_CODE_BEGIN);
}

void test_parser_label_resolution(void) {
    Tiny16Parser parser = {0};
    parser.source_filename = "test";

    // add a label
    strcpy(parser.symbols[0].name, "loop");
    parser.symbols[0].kind = TINY16_SYMBOL_LABEL;
    parser.symbols[0].value = 0x1000;
    parser.symbol_count = 1;

    // resolve the label
    uint16_t addr = tiny16_parser_label_addr(&parser, "loop", 4);
    assert(addr == 0x1000);

    // try non-existent label
    addr = tiny16_parser_label_addr(&parser, "notfound", 8);
    assert(addr == TINY16_PARSER_LABEL_NOT_FOUND);
}

void test_parser_duplicate_label(void) {
    const char* input = "start:\nstart:\n";
    Lexer lexer = lexer_new(input, strlen(input));
    Tiny16Parser parser = {0};
    parser.lexer = lexer;
    parser.source_filename = "test";
    parser.current_section = TINY16_PARSER_SECTION_CODE;
    parser.code_pc = TINY16_MEMORY_CODE_BEGIN;

    tiny16_parser_next(&parser);

    // first label should succeed
    bool result = tiny16_parser_parse_label(&parser);
    assert(result == true);
    assert(!tiny16_parser_has_error(&parser));

    tiny16_parser_skip_trivia(&parser);

    // second label (duplicate) should set an error
    result = tiny16_parser_parse_label(&parser);
    assert(result == true); // Still returns true (consumed the label)
    assert(tiny16_parser_has_error(&parser));
    assert(parser.error == TINY16_PARSER_ERROR_DUPLICATE_LABEL);
}

void test_parser_section_code(void) {
    const char* input = "section .code\n";
    Lexer lexer = lexer_new(input, strlen(input));
    Tiny16Parser parser = {0};
    parser.lexer = lexer;
    parser.source_filename = "test";
    parser.code_pc = TINY16_MEMORY_CODE_BEGIN;
    parser.current_section = TINY16_PARSER_SECTION_UNKNOWN;

    tiny16_parser_next(&parser);
    bool result = tiny16_parser_parse_section(&parser);

    assert(result == true);
    assert(parser.current_section == TINY16_PARSER_SECTION_CODE);
}

void test_parser_section_data(void) {
    const char* input = "section .data\n";
    Lexer lexer = lexer_new(input, strlen(input));
    Tiny16Parser parser = {0};
    parser.lexer = lexer;
    parser.source_filename = "test";
    parser.data_pc = TINY16_MEMORY_DATA_BEGIN;
    parser.current_section = TINY16_PARSER_SECTION_UNKNOWN;

    tiny16_parser_next(&parser);
    bool result = tiny16_parser_parse_section(&parser);

    assert(result == true);
    assert(parser.current_section == TINY16_PARSER_SECTION_DATA);
}

void test_parser_instruction_loadi(void) {
    const char* input = "loadi";
    Lexer lexer = lexer_new(input, strlen(input));
    Tiny16Parser parser = {0};
    parser.lexer = lexer;
    parser.source_filename = "test";

    tiny16_parser_next(&parser);
    Tiny16OpCode opcode = tiny16_parser_parse_mnemonic(&parser);

    assert(opcode == TINY16_OPCODE_LOADI);
}

void test_parser_instruction_arithmetic(void) {
    const char* tests[] = {"add", "sub", "and", "or", "xor"};
    Tiny16OpCode expected[] = {TINY16_OPCODE_ADD, TINY16_OPCODE_SUB, TINY16_OPCODE_AND,
                               TINY16_OPCODE_OR, TINY16_OPCODE_XOR};

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        Lexer lexer = lexer_new(tests[i], strlen(tests[i]));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        Tiny16OpCode opcode = tiny16_parser_parse_mnemonic(&parser);
        assert(opcode == expected[i]);
    }
}

void test_parser_instruction_jumps(void) {
    const char* tests[] = {"jmp", "jz", "jnz", "jc", "jnc"};
    Tiny16OpCode expected[] = {TINY16_OPCODE_JMP, TINY16_OPCODE_JZ, TINY16_OPCODE_JNZ,
                               TINY16_OPCODE_JC, TINY16_OPCODE_JNC};

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        Lexer lexer = lexer_new(tests[i], strlen(tests[i]));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        Tiny16OpCode opcode = tiny16_parser_parse_mnemonic(&parser);
        assert(opcode == expected[i]);
    }
}

void test_parser_instruction_load_store(void) {
    const char* tests[] = {"load", "store"};
    Tiny16OpCode expected[] = {TINY16_OPCODE_LOAD, TINY16_OPCODE_STORE};

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        Lexer lexer = lexer_new(tests[i], strlen(tests[i]));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        Tiny16OpCode opcode = tiny16_parser_parse_mnemonic(&parser);
        assert(opcode == expected[i]);
    }
}

void test_parser_register_parsing(void) {
    for (int i = 0; i < 8; i++) {
        char input[4];
        snprintf(input, sizeof(input), "r%d", i);

        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        uint8_t reg = tiny16_parser_parse_reg(&parser);
        assert(reg == i);
    }
}

void test_parser_register_pairs(void) {
    const char* tests[] = {"r0:r1", "r2:r3", "r4:r5", "r6:r7"};
    Tiny16AddrPair expected[] = {TINY16_ADDR_PAIR_01, TINY16_ADDR_PAIR_23, TINY16_ADDR_PAIR_45,
                                 TINY16_ADDR_PAIR_67};

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        Lexer lexer = lexer_new(tests[i], strlen(tests[i]));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        Tiny16AddrPair pair = tiny16_parser_parse_reg_pair(&parser);
        assert(pair == expected[i]);
    }
}

void test_parser_addressing_modes(void) {
    // Test base mode: r0, [r6:r7]
    {
        const char* input = "r0, [ r6:r7 ]";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        Tiny16Addr addr = tiny16_parser_parse_addr(&parser);
        assert(addr.reg == 0);
        assert(addr.mode == TINY16_ADDR_MODE_BASE);
        assert(addr.pair == TINY16_ADDR_PAIR_67);
    }

    // Test post-increment mode: r1, [r0:r1]+
    {
        const char* input = "r1, [ r0:r1 ] +";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        Tiny16Addr addr = tiny16_parser_parse_addr(&parser);
        assert(addr.reg == 1);
        assert(addr.mode == TINY16_ADDR_MODE_INC);
        assert(addr.pair == TINY16_ADDR_PAIR_01);
    }

    // Test post-decrement mode: r2, [r2:r3]-
    {
        const char* input = "r2, [ r2:r3 ] -";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        Tiny16Addr addr = tiny16_parser_parse_addr(&parser);
        assert(addr.reg == 2);
        assert(addr.mode == TINY16_ADDR_MODE_DEC);
        assert(addr.pair == TINY16_ADDR_PAIR_23);
    }

    // Test offset mode: r3, [r4:r5 + 10]
    {
        const char* input = "r3, [ r4:r5 + 10 ]";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        Tiny16Addr addr = tiny16_parser_parse_addr(&parser);
        assert(addr.reg == 3);
        assert(addr.mode == TINY16_ADDR_MODE_OFFSET);
        assert(addr.pair == TINY16_ADDR_PAIR_45);
        assert(addr.offset == 10);
    }
}

void test_parser_data_directive_db(void) {
    const char* input = "db 0x42\n";
    Lexer lexer = lexer_new(input, strlen(input));
    Tiny16Parser parser = {0};
    parser.lexer = lexer;
    parser.source_filename = "test";
    parser.current_section = TINY16_PARSER_SECTION_DATA;
    parser.data_pc = TINY16_MEMORY_DATA_BEGIN;

    tiny16_parser_next(&parser);
    tiny16_parser_parse_data(&parser);

    assert(parser.data_size == 1);
    assert(parser.data_buffer[0] == 0x42);
}

void test_parser_data_directive_string(void) {
    const char* input = "db \"AB\"\n";
    Lexer lexer = lexer_new(input, strlen(input));
    Tiny16Parser parser = {0};
    parser.lexer = lexer;
    parser.source_filename = "test";
    parser.current_section = TINY16_PARSER_SECTION_DATA;
    parser.data_pc = TINY16_MEMORY_DATA_BEGIN;

    tiny16_parser_next(&parser);
    tiny16_parser_parse_data(&parser);

    assert(parser.data_size == 2);
    assert(parser.data_buffer[0] == 'A');
    assert(parser.data_buffer[1] == 'B');
}

void test_parser_data_directive_times(void) {
    const char* input = "times 3 db 0xFF\n";
    Lexer lexer = lexer_new(input, strlen(input));
    Tiny16Parser parser = {0};
    parser.lexer = lexer;
    parser.source_filename = "test";
    parser.current_section = TINY16_PARSER_SECTION_DATA;
    parser.data_pc = TINY16_MEMORY_DATA_BEGIN;

    tiny16_parser_next(&parser);
    uint16_t times = tiny16_parser_parse_times_prefix(&parser);
    assert(times == 3);

    // Simulate what the assembler does: call parse_data multiple times
    for (uint16_t i = 0; i < times; ++i) {
        Lexer saved_lexer = parser.lexer;
        Token saved_token = parser.current_token;
        tiny16_parser_parse_data(&parser);
        if (i < times - 1) {
            parser.lexer = saved_lexer;
            parser.current_token = saved_token;
        }
    }

    assert(parser.data_size == 3);
    assert(parser.data_buffer[0] == 0xFF);
    assert(parser.data_buffer[1] == 0xFF);
    assert(parser.data_buffer[2] == 0xFF);
}

static Tiny16Parser run_pass1_data_only(const char* input) {
    Tiny16Parser parser = {0};
    parser.source_filename = "test";
    parser.current_section = TINY16_PARSER_SECTION_DATA;
    parser.data_pc = TINY16_DATA_BEGIN;
    parser.code_pc = TINY16_MEMORY_CODE_BEGIN;

    parser.lexer = lexer_new(input, strlen(input));
    tiny16_parser_next(&parser);

    while (parser.current_token.kind != TOKEN_END && !tiny16_parser_has_error(&parser)) {
        tiny16_parser_skip_trivia(&parser);
        if (parser.current_token.kind == TOKEN_END) break;

        if (tiny16_parser_parse_section(&parser)) {
            tiny16_parser_skip_to_eol(&parser);
            continue;
        }

        if (parser.current_section == TINY16_PARSER_SECTION_DATA) {
            if (tiny16_parser_parse_org(&parser)) {
                tiny16_parser_skip_to_eol(&parser);
                continue;
            }
        }

        if (tiny16_parser_parse_label(&parser)) {
            tiny16_parser_skip_trivia(&parser);
            if (parser.current_token.kind == TOKEN_END) break;
        }

        uint16_t times = tiny16_parser_parse_times_prefix(&parser);

        if (parser.current_section == TINY16_PARSER_SECTION_DATA) {
            if (parser.current_token.kind == TOKEN_KEYWORD) {
                for (uint16_t i = 0; i < times; ++i) {
                    Lexer saved_lexer = parser.lexer;
                    Token saved_token = parser.current_token;
                    tiny16_parser_parse_data(&parser);
                    if (i < times - 1) {
                        parser.lexer = saved_lexer;
                        parser.current_token = saved_token;
                    }
                }
            }
            tiny16_parser_skip_to_eol(&parser);
        } else {
            // Not used by these tests, but keep progress sane.
            parser.code_pc += 3 * times;
            tiny16_parser_skip_to_eol(&parser);
        }
    }

    return parser;
}

void test_parser_data_directive_org_pads_forward(void) {
    const char* input = "section .data\n"
                        "db 0x10, 0x11\n"
                        "org 0x4004\n"
                        "db 0x12\n";

    Tiny16Parser parser = run_pass1_data_only(input);
    assert(!tiny16_parser_has_error(&parser));
    assert(parser.data_size == 5);
    assert(parser.data_buffer[0] == 0x10);
    assert(parser.data_buffer[1] == 0x11);
    assert(parser.data_buffer[2] == 0x00);
    assert(parser.data_buffer[3] == 0x00);
    assert(parser.data_buffer[4] == 0x12);
    assert(parser.data_pc == (TINY16_DATA_BEGIN + parser.data_size));
}

void test_parser_data_directive_org_rewind_errors(void) {
    const char* input = "section .data\n"
                        "db 0x10\n"
                        "org 0x4000\n";

    Tiny16Parser parser = run_pass1_data_only(input);
    assert(tiny16_parser_has_error(&parser));
    assert(parser.error == TINY16_PARSER_ERROR_ORG_REWIND_NOT_ALLOWED);
}

void test_parser_data_directive_org_out_of_range_errors(void) {
    // Below data begin (0x4000)
    {
        const char* input = "section .data\n"
                            "org 0x3FFF\n";
        Tiny16Parser parser = run_pass1_data_only(input);
        assert(tiny16_parser_has_error(&parser));
        assert(parser.error == TINY16_PARSER_ERROR_OUT_OF_RANGE);
    }

    // Above data end (0x791F)
    {
        const char* input = "section .data\n"
                            "org 0x7920\n";
        Tiny16Parser parser = run_pass1_data_only(input);
        assert(tiny16_parser_has_error(&parser));
        assert(parser.error == TINY16_PARSER_ERROR_OUT_OF_RANGE);
    }
}

void test_parser_data_directive_org_updates_label_addr(void) {
    const char* input = "section .data\n"
                        "org 0x5000\n"
                        "tile0:\n"
                        "db 0xAA\n";

    Tiny16Parser parser = run_pass1_data_only(input);
    assert(!tiny16_parser_has_error(&parser));
    assert(parser.symbol_count == 1);
    assert(strcmp(parser.symbols[0].name, "tile0") == 0);
    assert(parser.symbols[0].kind == TINY16_SYMBOL_LABEL);
    assert(parser.symbols[0].value == 0x5000);
}

static Tiny16Parser run_pass1_symbols_only(const char* input) {
    Tiny16Parser parser = {0};
    parser.source_filename = "test";
    parser.current_section = TINY16_PARSER_SECTION_CODE;
    parser.code_pc = TINY16_MEMORY_CODE_BEGIN;
    parser.data_pc = TINY16_DATA_BEGIN;

    parser.lexer = lexer_new(input, strlen(input));
    tiny16_parser_next(&parser);

    while (parser.current_token.kind != TOKEN_END && !tiny16_parser_has_error(&parser)) {
        tiny16_parser_skip_trivia(&parser);
        if (parser.current_token.kind == TOKEN_END) break;

        if (tiny16_parser_parse_section(&parser)) {
            tiny16_parser_skip_to_eol(&parser);
            continue;
        }

        if (tiny16_parser_parse_const(&parser)) {
            tiny16_parser_skip_to_eol(&parser);
            continue;
        }

        if (tiny16_parser_parse_label(&parser)) {
            tiny16_parser_skip_to_eol(&parser);
            continue;
        }

        tiny16_parser_skip_to_eol(&parser);
    }

    return parser;
}

void test_parser_const_simple_number(void) {
    Tiny16Parser parser = run_pass1_symbols_only("X = 123\n");
    assert(!tiny16_parser_has_error(&parser));
    assert(parser.symbol_count == 1);
    assert(strcmp(parser.symbols[0].name, "X") == 0);
    assert(parser.symbols[0].kind == TINY16_SYMBOL_CONST);
    assert(parser.symbols[0].value == 123);
}

void test_parser_const_duplicate_errors(void) {
    Tiny16Parser parser = run_pass1_symbols_only("X = 1\nX = 2\n");
    assert(tiny16_parser_has_error(&parser));
    assert(parser.error == TINY16_PARSER_ERROR_DUPLICATE_CONST);
}

void test_parser_const_label_collision_errors(void) {
    // Const first, then label
    {
        Tiny16Parser parser = run_pass1_symbols_only("X = 1\nX:\n");
        assert(tiny16_parser_has_error(&parser));
        assert(parser.error == TINY16_PARSER_ERROR_SYMBOL_ALREADY_DEFINED);
    }

    // Label first, then const
    {
        Tiny16Parser parser = run_pass1_symbols_only("X:\nX = 1\n");
        assert(tiny16_parser_has_error(&parser));
        assert(parser.error == TINY16_PARSER_ERROR_SYMBOL_ALREADY_DEFINED);
    }
}

void test_parser_const_out_of_range_errors(void) {
    Tiny16Parser parser = run_pass1_symbols_only("X = 70000\n");
    assert(tiny16_parser_has_error(&parser));
    assert(parser.error == TINY16_PARSER_ERROR_OUT_OF_RANGE);
}

void test_parser_label_forward_reference(void) {
    Tiny16Parser parser = {0};
    parser.source_filename = "test";

    uint16_t addr = tiny16_parser_label_addr(&parser, "end", 3);
    assert(addr == TINY16_PARSER_LABEL_NOT_FOUND);

    strcpy(parser.symbols[0].name, "end");
    parser.symbols[0].kind = TINY16_SYMBOL_LABEL;
    parser.symbols[0].value = 0x2000;
    parser.symbol_count = 1;

    addr = tiny16_parser_label_addr(&parser, "end", 3);
    assert(addr == 0x2000);
}

void test_parser_label_backward_reference(void) {
    Tiny16Parser parser = {0};
    parser.source_filename = "test";

    strcpy(parser.symbols[0].name, "start");
    parser.symbols[0].kind = TINY16_SYMBOL_LABEL;
    parser.symbols[0].value = 0x1000;
    parser.symbol_count = 1;

    uint16_t addr = tiny16_parser_label_addr(&parser, "start", 5);
    assert(addr == 0x1000);
}

void test_parser_complete_program(void) {
    const char* input = "section .code\n"
                        "start:\n"
                        "  loadi r0, 10\n"
                        "  halt\n";

    Lexer lexer = lexer_new(input, strlen(input));
    Tiny16Parser parser = {0};
    parser.lexer = lexer;
    parser.source_filename = "test";
    parser.code_pc = TINY16_MEMORY_CODE_BEGIN;

    tiny16_parser_next(&parser);

    bool result = tiny16_parser_parse_section(&parser);
    assert(result == true);
    assert(parser.current_section == TINY16_PARSER_SECTION_CODE);

    tiny16_parser_skip_trivia(&parser);

    result = tiny16_parser_parse_label(&parser);
    assert(result == true);
    assert(parser.symbol_count == 1);
    assert(strcmp(parser.symbols[0].name, "start") == 0);
    assert(parser.symbols[0].kind == TINY16_SYMBOL_LABEL);
}

void test_parser_expr_simple_number(void) {
    const char* input = "42";
    Lexer lexer = lexer_new(input, strlen(input));
    Tiny16Parser parser = {0};
    parser.lexer = lexer;
    parser.source_filename = "test";

    tiny16_parser_next(&parser);
    long result = tiny16_parser_parse_expression(&parser);

    assert(!tiny16_parser_has_error(&parser));
    assert(result == 42);
}

void test_parser_expr_hex_binary_numbers(void) {
    // Hex number
    {
        const char* input = "0xFF";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 255);
    }

    // Binary number
    {
        const char* input = "0b1010";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 10);
    }
}

void test_parser_expr_symbol_reference(void) {
    const char* input = "SCREEN_WIDTH";
    Lexer lexer = lexer_new(input, strlen(input));
    Tiny16Parser parser = {0};
    parser.lexer = lexer;
    parser.source_filename = "test";

    // Add symbol
    strcpy(parser.symbols[0].name, "SCREEN_WIDTH");
    parser.symbols[0].kind = TINY16_SYMBOL_CONST;
    parser.symbols[0].value = 320;
    parser.symbol_count = 1;

    tiny16_parser_next(&parser);
    long result = tiny16_parser_parse_expression(&parser);

    assert(!tiny16_parser_has_error(&parser));
    assert(result == 320);
}

void test_parser_expr_addition_subtraction(void) {
    // Addition
    {
        const char* input = "10 + 5";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 15);
    }

    // Subtraction
    {
        const char* input = "10 - 3";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 7);
    }

    // Chained operations
    {
        const char* input = "100 - 20 + 5";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 85);
    }
}

void test_parser_expr_multiplication_division_modulo(void) {
    // Multiplication
    {
        const char* input = "8 * 4";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 32);
    }

    // Division
    {
        const char* input = "20 / 4";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 5);
    }

    // Modulo
    {
        const char* input = "17 % 5";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 2);
    }
}

void test_parser_expr_bitwise_operators(void) {
    // AND
    {
        const char* input = "0xFF & 0x0F";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 0x0F);
    }

    // OR
    {
        const char* input = "0xF0 | 0x0F";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 0xFF);
    }

    // XOR
    {
        const char* input = "0xFF ^ 0x0F";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 0xF0);
    }
}

void test_parser_expr_shift_operators(void) {
    // Left shift
    {
        const char* input = "1 << 4";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 16);
    }

    // Right shift
    {
        const char* input = "64 >> 2";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 16);
    }
}

void test_parser_expr_unary_minus(void) {
    // Simple unary minus
    {
        const char* input = "-42";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == -42);
    }

    // Unary minus in expression
    {
        const char* input = "10 + -5";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 5);
    }
}

void test_parser_expr_unary_not(void) {
    const char* input = "~0xFF";
    Lexer lexer = lexer_new(input, strlen(input));
    Tiny16Parser parser = {0};
    parser.lexer = lexer;
    parser.source_filename = "test";

    tiny16_parser_next(&parser);
    long result = tiny16_parser_parse_expression(&parser);

    assert(!tiny16_parser_has_error(&parser));
    assert(result == ~0xFF);
}

void test_parser_expr_parentheses(void) {
    // Basic parentheses
    {
        const char* input = "(5 + 3)";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 8);
    }

    // Parentheses changing precedence
    {
        const char* input = "(5 + 3) * 2";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 16);
    }

    // Nested parentheses
    {
        const char* input = "((10 + 5) * 2) - 3";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 27);
    }
}

void test_parser_expr_precedence(void) {
    // Multiplication before addition
    {
        const char* input = "2 + 3 * 4";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 14); // Not 20
    }

    // Shift before addition
    {
        const char* input = "1 + 2 << 2";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 12); // (1 + 2) << 2 = 3 << 2 = 12
    }

    // Bitwise AND before OR
    {
        const char* input = "0xFF | 0x0F & 0xF0";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 0xFF); // 0xFF | (0x0F & 0xF0) = 0xFF | 0 = 0xFF
    }
}

void test_parser_expr_complex(void) {
    // Complex expression with symbols
    {
        const char* input = "WIDTH * HEIGHT / 8";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        // Add symbols
        strcpy(parser.symbols[0].name, "WIDTH");
        parser.symbols[0].kind = TINY16_SYMBOL_CONST;
        parser.symbols[0].value = 320;
        strcpy(parser.symbols[1].name, "HEIGHT");
        parser.symbols[1].kind = TINY16_SYMBOL_CONST;
        parser.symbols[1].value = 200;
        parser.symbol_count = 2;

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 8000); // 320 * 200 / 8
    }

    // Complex expression with all operator types
    {
        const char* input = "(10 + 20) * 2 >> 2 & 0xFF";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        long result = tiny16_parser_parse_expression(&parser);

        assert(!tiny16_parser_has_error(&parser));
        assert(result == 15); // ((10 + 20) * 2) >> 2 & 0xFF = 60 >> 2 & 0xFF = 15 & 0xFF = 15
    }
}

void test_parser_expr_division_by_zero(void) {
    // Division by zero
    {
        const char* input = "10 / 0";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        tiny16_parser_parse_expression(&parser);

        assert(tiny16_parser_has_error(&parser));
        assert(parser.error == TINY16_PARSER_ERROR_INVALID_NUMBER);
    }

    // Modulo by zero
    {
        const char* input = "10 % 0";
        Lexer lexer = lexer_new(input, strlen(input));
        Tiny16Parser parser = {0};
        parser.lexer = lexer;
        parser.source_filename = "test";

        tiny16_parser_next(&parser);
        tiny16_parser_parse_expression(&parser);

        assert(tiny16_parser_has_error(&parser));
        assert(parser.error == TINY16_PARSER_ERROR_INVALID_NUMBER);
    }
}

void test_parser_expr_undefined_symbol(void) {
    const char* input = "UNDEFINED_SYMBOL";
    Lexer lexer = lexer_new(input, strlen(input));
    Tiny16Parser parser = {0};
    parser.lexer = lexer;
    parser.source_filename = "test";

    tiny16_parser_next(&parser);
    tiny16_parser_parse_expression(&parser);

    assert(tiny16_parser_has_error(&parser));
    assert(parser.error == TINY16_PARSER_ERROR_UNDEFINED_SYMBOL);
}

void test_integration_simple_program(void) {
    const char* input = "section .code\n"
                        "loadi r0, 42\n"
                        "halt\n";

    Lexer lexer = lexer_new(input, strlen(input));
    Tiny16Parser parser = {0};
    parser.lexer = lexer;
    parser.source_filename = "test_simple";
    parser.code_pc = TINY16_MEMORY_CODE_BEGIN;
    parser.current_section = TINY16_PARSER_SECTION_UNKNOWN;

    tiny16_parser_next(&parser);
}

void test_integration_loop_program(void) {
    const char* input = "section .code\n"
                        "  loadi r0, 5\n"
                        "loop:\n"
                        "  dec r0\n"
                        "  jnz loop\n"
                        "  halt\n";

    Lexer lexer = lexer_new(input, strlen(input));
    Tiny16Parser parser = {0};
    parser.lexer = lexer;
    parser.source_filename = "test_loop";
    parser.code_pc = TINY16_MEMORY_CODE_BEGIN;
    parser.current_section = TINY16_PARSER_SECTION_UNKNOWN;

    tiny16_parser_next(&parser);
}

void test_integration_subroutine_program(void) {
    const char* input = "section .code\n"
                        "  call func\n"
                        "  halt\n"
                        "func:\n"
                        "  loadi r0, 10\n"
                        "  ret\n";

    Lexer lexer = lexer_new(input, strlen(input));
    Tiny16Parser parser = {0};
    parser.lexer = lexer;
    parser.source_filename = "test_subroutine";
    parser.code_pc = TINY16_MEMORY_CODE_BEGIN;
    parser.current_section = TINY16_PARSER_SECTION_UNKNOWN;

    tiny16_parser_next(&parser);
}
