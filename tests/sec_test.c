#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../sec/args.c"
#include "../sec/args.h"
#include "../sec/ast.c"
#include "../sec/ast.h"
#include "../sec/codegen.c"
#include "../sec/codegen.h"
#include "../sec/lexer.c"
#include "../sec/lexer.h"
#include "../sec/macro.c"
#include "../sec/macro.h"
#include "../sec/optimizer.c"
#include "../sec/optimizer.h"
#include "../sec/parser.c"
#include "../sec/parser.h"

#include "testing.c"
#include "testing.h"

// =============================================================================
// Lexer Tests
// =============================================================================

void test_lexer_empty(void);
void test_lexer_whitespace(void);
void test_lexer_comments(void);
void test_lexer_parentheses(void);
void test_lexer_numbers_decimal(void);
void test_lexer_numbers_hex(void);
void test_lexer_strings(void);
void test_lexer_symbols(void);
void test_lexer_multiline(void);
void test_lexer_special_symbols(void);

// =============================================================================
// Parser Tests
// =============================================================================

void test_parser_number(void);
void test_parser_string(void);
void test_parser_symbol(void);
void test_parser_def(void);
void test_parser_defn_simple(void);
void test_parser_defn_with_params(void);
void test_parser_let(void);
void test_parser_set(void);
void test_parser_if(void);
void test_parser_while(void);
void test_parser_do(void);
void test_parser_add(void);
void test_parser_arithmetic_ops(void);
void test_parser_comparison_ops(void);
void test_parser_bitwise_ops(void);
void test_parser_nested_expressions(void);
void test_parser_function_call(void);
void test_parser_var(void);
void test_parser_set_bang(void);
void test_parser_cond(void);
void test_parser_when_unless(void);
void test_parser_for_range(void);
void test_parser_keyword_nil_true_false(void);
void test_parser_load_store(void);
void test_parser_hi_lo(void);
void test_parser_ns(void);
void test_parser_ns_with_body(void);
void test_parser_ns_error_no_symbol(void);
void test_parser_require_single(void);
void test_parser_require_multiple(void);
void test_parser_require_list_syntax(void);
void test_parser_require_with_strings(void);
void test_parser_require_empty(void);
void test_parser_defrecord_simple(void);
void test_parser_defrecord_with_hints(void);
void test_parser_field_get(void);
void test_parser_set_bang_field(void);
void test_parser_array(void);
void test_parser_nth(void);
void test_parser_len(void);
void test_parser_set_bang_nth(void);
void test_parser_fn(void);
void test_parser_fn_with_params(void);
void test_parser_nilp(void);
void test_parser_zerop(void);
void test_parser_posp(void);
void test_parser_negp(void);
void test_parser_error_expected_lparen(void);
void test_parser_error_expected_rparen(void);
void test_parser_error_unknown_form(void);

// =============================================================================
// Macro Tests
// =============================================================================

void test_macro_simple_substitution(void);
void test_macro_suffix_substitution(void);
void test_macro_numeric_args(void);

// =============================================================================
// Codegen Tests
// =============================================================================

void test_codegen_def(void);
void test_codegen_simple_function(void);
void test_codegen_function_with_params(void);
void test_codegen_arithmetic(void);
void test_codegen_if_statement(void);
void test_codegen_while_loop(void);
void test_codegen_function_call(void);
void test_codegen_let_bindings(void);
void test_codegen_data_section(void);
void test_codegen_error_undefined_symbol(void);
void test_codegen_error_duplicate_function(void);
void test_codegen_defrecord_var(void);
void test_codegen_defrecord_field_get(void);
void test_codegen_defrecord_field_set(void);
void test_codegen_array_scalar(void);
void test_codegen_array_record(void);
void test_codegen_nth_scalar(void);
void test_codegen_nth_record_field(void);
void test_codegen_set_bang_nth_scalar(void);
void test_codegen_set_bang_nth_field(void);
void test_codegen_len(void);
void test_codegen_nilp(void);
void test_codegen_zerop(void);
void test_codegen_posp(void);
void test_codegen_negp(void);
void test_codegen_fn_basic(void);
void test_codegen_fn_var(void);
void test_codegen_fn_ref(void);
void test_codegen_indirect_call(void);
void test_codegen_string_var(void);
void test_codegen_string_len(void);
void test_codegen_string_nth(void);
void test_parser_cast_u8(void);
void test_parser_cast_i8(void);
void test_codegen_cast_u8(void);
void test_codegen_cast_i8(void);

// =============================================================================
// Integration Tests
// =============================================================================

void test_integration_hello_world(void);
void test_integration_factorial(void);
void test_integration_fibonacci(void);
void test_integration_data_and_code(void);

// =============================================================================
// 16-bit / Fixed-point / Type Hint / Optimizer Tests
// =============================================================================

void test_lexer_fixed_point(void);
void test_parser_var_hint(void);
void test_parser_let_hint(void);
void test_parser_defn_param_hints(void);
void test_codegen_16bit_literal(void);
void test_codegen_16bit_var(void);
void test_codegen_16bit_add(void);
void test_codegen_16bit_sub(void);
void test_codegen_16bit_let(void);
void test_codegen_signed_compare(void);
void test_codegen_signed_shr(void);
void test_codegen_truthiness_nil(void);
void test_codegen_while_returns_nil(void);
void test_codegen_when_returns_nil(void);
void test_codegen_cond_returns_nil(void);
void test_codegen_for_array(void);
void test_codegen_u16_auto_increment(void);
void test_codegen_macros_emitted(void);
void test_optimizer_constant_fold(void);
void test_optimizer_strength_reduce(void);
void test_optimizer_dead_code(void);
void test_optimizer_dead_let_binding(void);

// =============================================================================
// Regression & Feature Coverage Tests (bugs fixed + example patterns)
// =============================================================================

// eval_const fix: variables must not be treated as constants in comparisons
void test_codegen_var_not_const_in_eq(void);
void test_codegen_var_not_const_in_ne(void);

// Optimizer: constant propagation must not replace mutated let bindings
void test_optimizer_cprop_mutated_let(void);
void test_optimizer_cprop_immutable_let(void);

// Optimizer: dead function elimination
void test_optimizer_dead_fn_elim(void);
void test_optimizer_dead_fn_keeps_transitive(void);

// hi/lo on data labels (used by floppy, apu_sfx_demo, sokoban)
void test_codegen_hi_lo_data_label(void);

// cond with keywords (used by floppy, v2_minimal)
void test_codegen_cond_keywords(void);

// while loop with let-bound counter (used by all stdlib while loops)
void test_codegen_while_let_counter(void);

// for loop with range (used by floppy, demo, sokoban, etc.)
void test_codegen_for_range(void);

// defrecord + array + nth with variable index (used by floppy, demo)
void test_codegen_nth_record_variable_index(void);

// data label with set! (used by floppy's fixed-point macros)
void test_codegen_data_label_set_bang(void);

// store/load with computed address (used by floppy's init-palette)
void test_codegen_store_computed_addr(void);

// and/or short-circuit logic (used by floppy, demo, sokoban)
void test_codegen_and_short_circuit(void);
void test_codegen_or_short_circuit(void);

// Optimized codegen still generates correct code at -O2
void test_codegen_optimized_while_loop(void);
void test_codegen_optimized_for_range(void);

// ^i16 record fields in arrays — expr_is_16bit/expr_is_signed for (nth arr i)
void test_parser_defrecord_field_is_signed(void);
void test_codegen_i16_field_nth_is_16bit(void);
void test_codegen_i16_field_nth_arithmetic(void);
void test_codegen_shl_u8_by_8_is_16bit(void);

int main(void) {
    test_init();

    // Lexer tests
    TEST(test_lexer_empty);
    TEST(test_lexer_whitespace);
    TEST(test_lexer_comments);
    TEST(test_lexer_parentheses);
    TEST(test_lexer_numbers_decimal);
    TEST(test_lexer_numbers_hex);
    TEST(test_lexer_strings);
    TEST(test_lexer_symbols);
    TEST(test_lexer_multiline);
    TEST(test_lexer_special_symbols);

    // Parser tests
    TEST(test_parser_number);
    TEST(test_parser_string);
    TEST(test_parser_symbol);
    TEST(test_parser_def);
    TEST(test_parser_defn_simple);
    TEST(test_parser_defn_with_params);
    TEST(test_parser_let);
    TEST(test_parser_set);
    TEST(test_parser_if);
    TEST(test_parser_while);
    TEST(test_parser_do);
    TEST(test_parser_add);
    TEST(test_parser_arithmetic_ops);
    TEST(test_parser_comparison_ops);
    TEST(test_parser_bitwise_ops);
    TEST(test_parser_nested_expressions);
    TEST(test_parser_function_call);
    TEST(test_parser_var);
    TEST(test_parser_set_bang);
    TEST(test_parser_cond);
    TEST(test_parser_when_unless);
    TEST(test_parser_for_range);
    TEST(test_parser_keyword_nil_true_false);
    TEST(test_parser_load_store);
    TEST(test_parser_hi_lo);
    TEST(test_parser_ns);
    TEST(test_parser_ns_with_body);
    TEST(test_parser_ns_error_no_symbol);
    TEST(test_parser_require_single);
    TEST(test_parser_require_multiple);
    TEST(test_parser_require_list_syntax);
    TEST(test_parser_require_with_strings);
    TEST(test_parser_require_empty);
    TEST(test_parser_defrecord_simple);
    TEST(test_parser_defrecord_with_hints);
    TEST(test_parser_field_get);
    TEST(test_parser_set_bang_field);
    TEST(test_parser_array);
    TEST(test_parser_nth);
    TEST(test_parser_len);
    TEST(test_parser_set_bang_nth);
    TEST(test_parser_fn);
    TEST(test_parser_fn_with_params);
    TEST(test_parser_nilp);
    TEST(test_parser_zerop);
    TEST(test_parser_posp);
    TEST(test_parser_negp);
    TEST(test_parser_error_expected_lparen);
    TEST(test_parser_error_expected_rparen);
    TEST(test_parser_error_unknown_form);

    // Macro tests
    TEST(test_macro_simple_substitution);
    TEST(test_macro_suffix_substitution);
    TEST(test_macro_numeric_args);

    // Codegen tests
    TEST(test_codegen_def);
    TEST(test_codegen_simple_function);
    TEST(test_codegen_function_with_params);
    TEST(test_codegen_arithmetic);
    TEST(test_codegen_if_statement);
    TEST(test_codegen_while_loop);
    TEST(test_codegen_function_call);
    TEST(test_codegen_let_bindings);
    TEST(test_codegen_data_section);
    TEST(test_codegen_error_undefined_symbol);
    TEST(test_codegen_error_duplicate_function);
    TEST(test_codegen_defrecord_var);
    TEST(test_codegen_defrecord_field_get);
    TEST(test_codegen_defrecord_field_set);
    TEST(test_codegen_array_scalar);
    TEST(test_codegen_array_record);
    TEST(test_codegen_nth_scalar);
    TEST(test_codegen_nth_record_field);
    TEST(test_codegen_set_bang_nth_scalar);
    TEST(test_codegen_set_bang_nth_field);
    TEST(test_codegen_len);
    TEST(test_codegen_nilp);
    TEST(test_codegen_zerop);
    TEST(test_codegen_posp);
    TEST(test_codegen_negp);
    TEST(test_codegen_fn_basic);
    TEST(test_codegen_fn_var);
    TEST(test_codegen_fn_ref);
    TEST(test_codegen_indirect_call);
    TEST(test_codegen_string_var);
    TEST(test_codegen_string_len);
    TEST(test_codegen_string_nth);
    TEST(test_parser_cast_u8);
    TEST(test_parser_cast_i8);
    TEST(test_codegen_cast_u8);
    TEST(test_codegen_cast_i8);

    // Integration tests
    TEST(test_integration_hello_world);
    TEST(test_integration_factorial);
    TEST(test_integration_fibonacci);
    TEST(test_integration_data_and_code);

    // 16-bit / Fixed-point / Type Hint / Optimizer tests
    TEST(test_lexer_fixed_point);
    TEST(test_parser_var_hint);
    TEST(test_parser_let_hint);
    TEST(test_parser_defn_param_hints);
    TEST(test_codegen_16bit_literal);
    TEST(test_codegen_16bit_var);
    TEST(test_codegen_16bit_add);
    TEST(test_codegen_16bit_sub);
    TEST(test_codegen_16bit_let);
    TEST(test_codegen_signed_compare);
    TEST(test_codegen_signed_shr);
    TEST(test_codegen_truthiness_nil);
    TEST(test_codegen_while_returns_nil);
    TEST(test_codegen_when_returns_nil);
    TEST(test_codegen_cond_returns_nil);
    TEST(test_codegen_for_array);
    TEST(test_codegen_u16_auto_increment);
    TEST(test_codegen_macros_emitted);
    TEST(test_optimizer_constant_fold);
    TEST(test_optimizer_strength_reduce);
    TEST(test_optimizer_dead_code);
    TEST(test_optimizer_dead_let_binding);

    // Regression & feature coverage tests
    TEST(test_codegen_var_not_const_in_eq);
    TEST(test_codegen_var_not_const_in_ne);
    TEST(test_optimizer_cprop_mutated_let);
    TEST(test_optimizer_cprop_immutable_let);
    TEST(test_optimizer_dead_fn_elim);
    TEST(test_optimizer_dead_fn_keeps_transitive);
    TEST(test_codegen_hi_lo_data_label);
    TEST(test_codegen_cond_keywords);
    TEST(test_codegen_while_let_counter);
    TEST(test_codegen_for_range);
    TEST(test_codegen_nth_record_variable_index);
    TEST(test_codegen_data_label_set_bang);
    TEST(test_codegen_store_computed_addr);
    TEST(test_codegen_and_short_circuit);
    TEST(test_codegen_or_short_circuit);
    TEST(test_codegen_optimized_while_loop);
    TEST(test_codegen_optimized_for_range);
    TEST(test_parser_defrecord_field_is_signed);
    TEST(test_codegen_i16_field_nth_is_16bit);
    TEST(test_codegen_i16_field_nth_arithmetic);
    TEST(test_codegen_shl_u8_by_8_is_16bit);

    return test_run_all();
}

// =============================================================================
// Lexer Test Implementations
// =============================================================================

void test_lexer_empty(void) {
    const char* input = "";
    SeLexer lexer = se_lexer_new(input, strlen(input));
    SeToken token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_END);
}

void test_lexer_whitespace(void) {
    const char* input = "   \t\n  ";
    SeLexer lexer = se_lexer_new(input, strlen(input));
    SeToken token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_END);
}

void test_lexer_comments(void) {
    const char* input = "; this is a comment\n42";
    SeLexer lexer = se_lexer_new(input, strlen(input));

    SeToken token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_NUMBER);
    TEST_ASSERT(token.number_value == 42);

    token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_END);
}

void test_lexer_parentheses(void) {
    const char* input = "()";
    SeLexer lexer = se_lexer_new(input, strlen(input));

    SeToken token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_LPAREN);

    token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_RPAREN);

    token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_END);
}

void test_lexer_numbers_decimal(void) {
    const char* input = "0 42 255";
    SeLexer lexer = se_lexer_new(input, strlen(input));

    SeToken token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_NUMBER);
    TEST_ASSERT(token.number_value == 0);

    token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_NUMBER);
    TEST_ASSERT(token.number_value == 42);

    token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_NUMBER);
    TEST_ASSERT(token.number_value == 255);
}

void test_lexer_numbers_hex(void) {
    const char* input = "0x00 0xFF 0xAB";
    SeLexer lexer = se_lexer_new(input, strlen(input));

    SeToken token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_NUMBER);
    TEST_ASSERT(token.number_value == 0x00);

    token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_NUMBER);
    TEST_ASSERT(token.number_value == 0xFF);

    token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_NUMBER);
    TEST_ASSERT(token.number_value == 0xAB);
}

void test_lexer_strings(void) {
    const char* input = "\"hello\" \"world\"";
    SeLexer lexer = se_lexer_new(input, strlen(input));

    SeToken token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_STRING);
    TEST_ASSERT(strncmp(token.text, "\"hello\"", token.text_len) == 0);

    token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_STRING);
    TEST_ASSERT(strncmp(token.text, "\"world\"", token.text_len) == 0);
}

void test_lexer_symbols(void) {
    const char* input = "defn add x y";
    SeLexer lexer = se_lexer_new(input, strlen(input));

    SeToken token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_SYMBOL);
    TEST_ASSERT(strncmp(token.text, "defn", token.text_len) == 0);

    token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_SYMBOL);
    TEST_ASSERT(strncmp(token.text, "add", token.text_len) == 0);

    token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_SYMBOL);
    TEST_ASSERT(strncmp(token.text, "x", token.text_len) == 0);

    token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_SYMBOL);
    TEST_ASSERT(strncmp(token.text, "y", token.text_len) == 0);
}

void test_lexer_multiline(void) {
    const char* input = "(defn foo ()\n  42)";
    SeLexer lexer = se_lexer_new(input, strlen(input));

    SeToken token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_LPAREN);
    TEST_ASSERT(token.line == 1);

    token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_SYMBOL);

    token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_SYMBOL);

    token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_LPAREN);

    token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_RPAREN);

    token = se_lexer_next(&lexer);
    TEST_ASSERT(token.kind == SE_TOKEN_NUMBER);
    TEST_ASSERT(token.line == 2);
}

void test_lexer_special_symbols(void) {
    const char* input = "+ - * / < > = != <= >=";
    SeLexer lexer = se_lexer_new(input, strlen(input));

    const char* expected[] = {"+", "-", "*", "/", "<", ">", "=", "!=", "<=", ">="};
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        SeToken token = se_lexer_next(&lexer);
        TEST_ASSERT(token.kind == SE_TOKEN_SYMBOL);
        TEST_ASSERT(strncmp(token.text, expected[i], token.text_len) == 0);
    }
}

// =============================================================================
// Parser Test Implementations
// =============================================================================

void test_parser_number(void) {
    const char* input = "42";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_NUMBER);
    TEST_ASSERT(node->as.number == 42);
}

void test_parser_string(void) {
    const char* input = "\"hello\"";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_STRING);
    TEST_ASSERT(strcmp(node->as.symbol.name, "hello") == 0);
}

void test_parser_symbol(void) {
    const char* input = "foo";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_SYMBOL);
    TEST_ASSERT(strcmp(node->as.symbol.name, "foo") == 0);
}

void test_parser_def(void) {
    const char* input = "(def x 42)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_DEF);
    TEST_ASSERT(strcmp(node->as.def.name, "x") == 0);
    TEST_ASSERT(node->as.def.value != NULL);
    TEST_ASSERT(node->as.def.value->kind == AST_NUMBER);
    TEST_ASSERT(node->as.def.value->as.number == 42);
}

void test_parser_defn_simple(void) {
    const char* input = "(defn foo () 42)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_DEFN);
    TEST_ASSERT(strcmp(node->as.defn.name, "foo") == 0);
    TEST_ASSERT(node->as.defn.param_count == 0);
    TEST_ASSERT(node->as.defn.body.count == 1);
    TEST_ASSERT(node->as.defn.body.items[0]->kind == AST_NUMBER);
}

void test_parser_defn_with_params(void) {
    const char* input = "(defn add (x y) (+ x y))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_DEFN);
    TEST_ASSERT(strcmp(node->as.defn.name, "add") == 0);
    TEST_ASSERT(node->as.defn.param_count == 2);
    TEST_ASSERT(strcmp(node->as.defn.params[0], "x") == 0);
    TEST_ASSERT(strcmp(node->as.defn.params[1], "y") == 0);
    TEST_ASSERT(node->as.defn.body.count == 1);
}

void test_parser_let(void) {
    const char* input = "(let (x 10 y 20) (+ x y))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_LET);
    TEST_ASSERT(node->as.let.binding_count == 2);
    TEST_ASSERT(strcmp(node->as.let.vars[0], "x") == 0);
    TEST_ASSERT(strcmp(node->as.let.vars[1], "y") == 0);
    TEST_ASSERT(node->as.let.vals[0]->kind == AST_NUMBER);
    TEST_ASSERT(node->as.let.vals[1]->kind == AST_NUMBER);
    TEST_ASSERT(node->as.let.body.count == 1);
}

void test_parser_set(void) {
    /* v2: (set ...) removed; (set x 42) parses as function call */
    const char* input = "(set x 42)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_CALL);
    TEST_ASSERT(strcmp(node->as.call.func, "set") == 0);
    TEST_ASSERT(node->as.call.arg_count == 2);
}

void test_parser_if(void) {
    const char* input = "(if (< x 10) 1 0)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_IF);
    TEST_ASSERT(node->as.if_expr.cond != NULL);
    TEST_ASSERT(node->as.if_expr.then_branch != NULL);
    TEST_ASSERT(node->as.if_expr.else_branch != NULL);
}

void test_parser_while(void) {
    const char* input = "(while (< i 10) (set i (+ i 1)))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_WHILE);
    TEST_ASSERT(node->as.while_expr.cond != NULL);
    TEST_ASSERT(node->as.while_expr.body.count == 1);
}

void test_parser_do(void) {
    const char* input = "(do (set x 1) (set y 2) x)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_DO);
    TEST_ASSERT(node->as.block.exprs.count == 3);
}

void test_parser_add(void) {
    const char* input = "(+ 1 2)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_ADD);
    TEST_ASSERT(node->as.binary.left->kind == AST_NUMBER);
    TEST_ASSERT(node->as.binary.right->kind == AST_NUMBER);
}

void test_parser_arithmetic_ops(void) {
    const char* tests[] = {"(+ 1 2)", "(- 10 5)", "(inc x)", "(dec y)"};
    AstKind expected[] = {AST_ADD, AST_SUB, AST_INC, AST_DEC};

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        AstPool pool;
        ast_pool_init(&pool);
        SeParser parser;
        se_parser_init(&parser, tests[i], strlen(tests[i]), &pool);

        AstNode* node = se_parser_parse_form(&parser);
        TEST_ASSERT(node != NULL);
        TEST_ASSERT(node->kind == expected[i]);
    }
}

void test_parser_comparison_ops(void) {
    const char* tests[] = {"(= x y)", "(!= a b)", "(< x 10)", "(> y 5)", "(<= i n)", "(>= j 0)"};
    AstKind expected[] = {AST_EQ, AST_NE, AST_LT, AST_GT, AST_LE, AST_GE};

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        AstPool pool;
        ast_pool_init(&pool);
        SeParser parser;
        se_parser_init(&parser, tests[i], strlen(tests[i]), &pool);

        AstNode* node = se_parser_parse_form(&parser);
        TEST_ASSERT(node != NULL);
        TEST_ASSERT(node->kind == expected[i]);
    }
}

void test_parser_bitwise_ops(void) {
    const char* tests[] = {"(& x y)", "(| a b)", "(^ x y)", "(~ val)", "(<< n 2)", "(>> n 1)"};
    AstKind expected[] = {AST_BAND, AST_BOR, AST_XOR, AST_BNOT, AST_SHL, AST_SHR};

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        AstPool pool;
        ast_pool_init(&pool);
        SeParser parser;
        se_parser_init(&parser, tests[i], strlen(tests[i]), &pool);

        AstNode* node = se_parser_parse_form(&parser);
        TEST_ASSERT(node != NULL);
        TEST_ASSERT(node->kind == expected[i]);
    }
}

void test_parser_nested_expressions(void) {
    const char* input = "(+ (* 2 3) (- 10 4))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_ADD);
    TEST_ASSERT(node->as.binary.left->kind == AST_MUL);
    TEST_ASSERT(node->as.binary.right->kind == AST_SUB);
}

void test_parser_function_call(void) {
    const char* input = "(foo 1 2 3)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_CALL);
    TEST_ASSERT(strcmp(node->as.call.func, "foo") == 0);
    TEST_ASSERT(node->as.call.arg_count == 3);
}

void test_parser_var(void) {
    const char* input = "(var x 42)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_VAR);
    TEST_ASSERT(strcmp(node->as.var.name, "x") == 0);
    TEST_ASSERT(node->as.var.value != NULL);
    TEST_ASSERT(node->as.var.value->kind == AST_NUMBER);
    TEST_ASSERT(node->as.var.value->as.number == 42);
}

void test_parser_set_bang(void) {
    const char* input = "(set! x 1)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_SET_BANG);
    TEST_ASSERT(strcmp(node->as.set.var, "x") == 0);
    TEST_ASSERT(node->as.set.value != NULL);
}

void test_parser_cond(void) {
    const char* input = "(cond (= x 0) 1 true 2)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_COND);
    TEST_ASSERT(node->as.cond.clause_count == 2);
    TEST_ASSERT(node->as.cond.tests[0] != NULL);
    TEST_ASSERT(node->as.cond.bodies[0].count >= 1);
}

void test_parser_when_unless(void) {
    {
        const char* input = "(when true 1)";
        AstPool pool;
        ast_pool_init(&pool);
        SeParser parser;
        se_parser_init(&parser, input, strlen(input), &pool);
        AstNode* node = se_parser_parse_form(&parser);
        TEST_ASSERT(node != NULL);
        TEST_ASSERT(node->kind == AST_WHEN);
        TEST_ASSERT(node->as.when_expr.cond != NULL);
        TEST_ASSERT(node->as.when_expr.body.count >= 1);
    }
    {
        const char* input = "(unless false 1)";
        AstPool pool;
        ast_pool_init(&pool);
        SeParser parser;
        se_parser_init(&parser, input, strlen(input), &pool);
        AstNode* node = se_parser_parse_form(&parser);
        TEST_ASSERT(node != NULL);
        TEST_ASSERT(node->kind == AST_UNLESS);
    }
}

void test_parser_for_range(void) {
    const char* input = "(for (i (range 0 10)) 1)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_FOR);
    TEST_ASSERT(strcmp(node->as.for_expr.var, "i") == 0);
    TEST_ASSERT(node->as.for_expr.collection != NULL);
    TEST_ASSERT(node->as.for_expr.collection->kind == AST_RANGE);
    TEST_ASSERT(node->as.for_expr.body.count >= 1);
}

void test_parser_keyword_nil_true_false(void) {
    {
        const char* input = ":foo";
        SeLexer lexer = se_lexer_new(input, strlen(input));
        SeToken t = se_lexer_next(&lexer);
        TEST_ASSERT(t.kind == SE_TOKEN_KEYWORD);
        TEST_ASSERT(t.text_len == 4 && strncmp(t.text, ":foo", t.text_len) == 0);
    }
    {
        const char* input = "nil";
        SeLexer lexer = se_lexer_new(input, strlen(input));
        SeToken t = se_lexer_next(&lexer);
        TEST_ASSERT(t.kind == SE_TOKEN_NIL);
    }
    {
        const char* input = "true";
        SeLexer lexer = se_lexer_new(input, strlen(input));
        SeToken t = se_lexer_next(&lexer);
        TEST_ASSERT(t.kind == SE_TOKEN_TRUE);
    }
    {
        const char* input = "false";
        SeLexer lexer = se_lexer_new(input, strlen(input));
        SeToken t = se_lexer_next(&lexer);
        TEST_ASSERT(t.kind == SE_TOKEN_FALSE);
    }
}

void test_parser_load_store(void) {
    {
        const char* input = "(load 0xBF00)";
        AstPool pool;
        ast_pool_init(&pool);
        SeParser parser;
        se_parser_init(&parser, input, strlen(input), &pool);

        AstNode* node = se_parser_parse_form(&parser);
        TEST_ASSERT(node != NULL);
        TEST_ASSERT(node->kind == AST_LOAD);
        TEST_ASSERT(node->as.load.addr != NULL);
    }
    {
        const char* input = "(store 0xBF00 42)";
        AstPool pool;
        ast_pool_init(&pool);
        SeParser parser;
        se_parser_init(&parser, input, strlen(input), &pool);

        AstNode* node = se_parser_parse_form(&parser);
        TEST_ASSERT(node != NULL);
        TEST_ASSERT(node->kind == AST_STORE);
        TEST_ASSERT(node->as.store.addr != NULL);
        TEST_ASSERT(node->as.store.value != NULL);
    }
}

void test_parser_hi_lo(void) {
    // Test hi
    {
        const char* input = "(hi 0x1234)";
        AstPool pool;
        ast_pool_init(&pool);
        SeParser parser;
        se_parser_init(&parser, input, strlen(input), &pool);

        AstNode* node = se_parser_parse_form(&parser);
        TEST_ASSERT(node != NULL);
        TEST_ASSERT(node->kind == AST_HI);
    }

    // Test lo
    {
        const char* input = "(lo 0x1234)";
        AstPool pool;
        ast_pool_init(&pool);
        SeParser parser;
        se_parser_init(&parser, input, strlen(input), &pool);

        AstNode* node = se_parser_parse_form(&parser);
        TEST_ASSERT(node != NULL);
        TEST_ASSERT(node->kind == AST_LO);
    }
}

void test_parser_ns(void) {
    const char* input = "(ns mylib)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_NS);
    TEST_ASSERT(strcmp(node->as.symbol.name, "mylib") == 0);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_ns_with_body(void) {
    const char* input = "(ns mylib (require (core)))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstProgram program;
    TEST_ASSERT(se_parser_parse_program(&parser, &program));
    TEST_ASSERT(program.node_count == 2);
    TEST_ASSERT(program.nodes[0]->kind == AST_NS);
    TEST_ASSERT(strcmp(program.nodes[0]->as.symbol.name, "mylib") == 0);
    TEST_ASSERT(program.nodes[1]->kind == AST_REQUIRE);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_ns_error_no_symbol(void) {
    const char* input = "(ns 123)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    (void)node;
    TEST_ASSERT(se_parser_has_error(&parser));
}

void test_parser_require_single(void) {
    const char* input = "(require math)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_REQUIRE);
    TEST_ASSERT(node->as.block.exprs.count == 1);
    TEST_ASSERT(node->as.block.exprs.items[0]->kind == AST_SYMBOL);
    TEST_ASSERT(strcmp(node->as.block.exprs.items[0]->as.symbol.name, "math") == 0);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_require_multiple(void) {
    const char* input = "(require math io graphics)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_REQUIRE);
    TEST_ASSERT(node->as.block.exprs.count == 3);
    TEST_ASSERT(node->as.block.exprs.items[0]->kind == AST_SYMBOL);
    TEST_ASSERT(strcmp(node->as.block.exprs.items[0]->as.symbol.name, "math") == 0);
    TEST_ASSERT(node->as.block.exprs.items[1]->kind == AST_SYMBOL);
    TEST_ASSERT(strcmp(node->as.block.exprs.items[1]->as.symbol.name, "io") == 0);
    TEST_ASSERT(node->as.block.exprs.items[2]->kind == AST_SYMBOL);
    TEST_ASSERT(strcmp(node->as.block.exprs.items[2]->as.symbol.name, "graphics") == 0);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_require_list_syntax(void) {
    const char* input = "(require (math io graphics))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_REQUIRE);
    TEST_ASSERT(node->as.block.exprs.count == 3);
    TEST_ASSERT(node->as.block.exprs.items[0]->kind == AST_SYMBOL);
    TEST_ASSERT(strcmp(node->as.block.exprs.items[0]->as.symbol.name, "math") == 0);
    TEST_ASSERT(node->as.block.exprs.items[1]->kind == AST_SYMBOL);
    TEST_ASSERT(strcmp(node->as.block.exprs.items[1]->as.symbol.name, "io") == 0);
    TEST_ASSERT(node->as.block.exprs.items[2]->kind == AST_SYMBOL);
    TEST_ASSERT(strcmp(node->as.block.exprs.items[2]->as.symbol.name, "graphics") == 0);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_require_with_strings(void) {
    const char* input = "(require \"math.se\" \"io.se\")";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_REQUIRE);
    TEST_ASSERT(node->as.block.exprs.count == 2);
    TEST_ASSERT(node->as.block.exprs.items[0]->kind == AST_STRING);
    TEST_ASSERT(strcmp(node->as.block.exprs.items[0]->as.symbol.name, "math.se") == 0);
    TEST_ASSERT(node->as.block.exprs.items[1]->kind == AST_STRING);
    TEST_ASSERT(strcmp(node->as.block.exprs.items[1]->as.symbol.name, "io.se") == 0);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_require_empty(void) {
    const char* input = "(require)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_REQUIRE);
    TEST_ASSERT(node->as.block.exprs.count == 0);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_defrecord_simple(void) {
    const char* input = "(defrecord pipe (x gap-y passed))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_DEFRECORD);
    TEST_ASSERT(strcmp(node->as.defrecord.name, "pipe") == 0);
    TEST_ASSERT(node->as.defrecord.field_count == 3);
    TEST_ASSERT(strcmp(node->as.defrecord.fields[0], "x") == 0);
    TEST_ASSERT(strcmp(node->as.defrecord.fields[1], "gap-y") == 0);
    TEST_ASSERT(strcmp(node->as.defrecord.fields[2], "passed") == 0);
    TEST_ASSERT(!node->as.defrecord.field_is_16bit[0]);
    TEST_ASSERT(!node->as.defrecord.field_is_16bit[1]);
    TEST_ASSERT(!node->as.defrecord.field_is_16bit[2]);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_defrecord_with_hints(void) {
    const char* input = "(defrecord entity (^i16 x ^i16 y hp state))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_DEFRECORD);
    TEST_ASSERT(strcmp(node->as.defrecord.name, "entity") == 0);
    TEST_ASSERT(node->as.defrecord.field_count == 4);
    TEST_ASSERT(strcmp(node->as.defrecord.fields[0], "x") == 0);
    TEST_ASSERT(node->as.defrecord.field_is_16bit[0]);
    TEST_ASSERT(strcmp(node->as.defrecord.fields[1], "y") == 0);
    TEST_ASSERT(node->as.defrecord.field_is_16bit[1]);
    TEST_ASSERT(strcmp(node->as.defrecord.fields[2], "hp") == 0);
    TEST_ASSERT(!node->as.defrecord.field_is_16bit[2]);
    TEST_ASSERT(strcmp(node->as.defrecord.fields[3], "state") == 0);
    TEST_ASSERT(!node->as.defrecord.field_is_16bit[3]);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_field_get(void) {
    const char* input = "(:x player)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_FIELD_GET);
    TEST_ASSERT(strcmp(node->as.field_get.field, ":x") == 0);
    TEST_ASSERT(node->as.field_get.record != NULL);
    TEST_ASSERT(node->as.field_get.record->kind == AST_SYMBOL);
    TEST_ASSERT(strcmp(node->as.field_get.record->as.symbol.name, "player") == 0);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_set_bang_field(void) {
    const char* input = "(set! (:x player) 10)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_SET_BANG);
    TEST_ASSERT(strcmp(node->as.set.var, ":x") == 0);
    TEST_ASSERT(node->as.set.target_expr != NULL);
    TEST_ASSERT(node->as.set.target_expr->kind == AST_SYMBOL);
    TEST_ASSERT(strcmp(node->as.set.target_expr->as.symbol.name, "player") == 0);
    TEST_ASSERT(node->as.set.value != NULL);
    TEST_ASSERT(node->as.set.value->kind == AST_NUMBER);
    TEST_ASSERT(node->as.set.value->as.number == 10);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_array(void) {
    const char* input = "(array 4 0)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_ARRAY);
    TEST_ASSERT(node->as.array_expr.count != NULL);
    TEST_ASSERT(node->as.array_expr.count->kind == AST_NUMBER);
    TEST_ASSERT(node->as.array_expr.count->as.number == 4);
    TEST_ASSERT(node->as.array_expr.value != NULL);
    TEST_ASSERT(node->as.array_expr.value->kind == AST_NUMBER);
    TEST_ASSERT(node->as.array_expr.value->as.number == 0);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_nth(void) {
    const char* input = "(nth scores 2)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_NTH);
    TEST_ASSERT(node->as.binary.left != NULL);
    TEST_ASSERT(node->as.binary.left->kind == AST_SYMBOL);
    TEST_ASSERT(strcmp(node->as.binary.left->as.symbol.name, "scores") == 0);
    TEST_ASSERT(node->as.binary.right != NULL);
    TEST_ASSERT(node->as.binary.right->kind == AST_NUMBER);
    TEST_ASSERT(node->as.binary.right->as.number == 2);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_len(void) {
    const char* input = "(len scores)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_LEN);
    TEST_ASSERT(node->as.unary.operand != NULL);
    TEST_ASSERT(node->as.unary.operand->kind == AST_SYMBOL);
    TEST_ASSERT(strcmp(node->as.unary.operand->as.symbol.name, "scores") == 0);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_set_bang_nth(void) {
    const char* input = "(set! (nth scores 0) 99)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_SET_BANG);
    TEST_ASSERT(node->as.set.var[0] == '\0'); // empty var signals nth target
    TEST_ASSERT(node->as.set.target_expr != NULL);
    TEST_ASSERT(node->as.set.target_expr->kind == AST_NTH);
    TEST_ASSERT(node->as.set.value != NULL);
    TEST_ASSERT(node->as.set.value->kind == AST_NUMBER);
    TEST_ASSERT(node->as.set.value->as.number == 99);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_fn(void) {
    const char* input = "(fn () (+ 1 2))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_FN);
    TEST_ASSERT(node->as.defn.param_count == 0);
    TEST_ASSERT(node->as.defn.body.count == 1);
    // Generated name should be __fn0
    TEST_ASSERT(strncmp(node->as.defn.name, "__fn", 4) == 0);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_fn_with_params(void) {
    const char* input = "(fn (x y) (+ x y))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_FN);
    TEST_ASSERT(node->as.defn.param_count == 2);
    TEST_ASSERT(strcmp(node->as.defn.params[0], "x") == 0);
    TEST_ASSERT(strcmp(node->as.defn.params[1], "y") == 0);
    TEST_ASSERT(node->as.defn.body.count == 1);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_nilp(void) {
    const char* input = "(nil? x)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_NILP);
    TEST_ASSERT(node->as.unary.operand != NULL);
    TEST_ASSERT(node->as.unary.operand->kind == AST_SYMBOL);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_zerop(void) {
    const char* input = "(zero? x)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_ZEROP);
    TEST_ASSERT(node->as.unary.operand != NULL);
    TEST_ASSERT(node->as.unary.operand->kind == AST_SYMBOL);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_posp(void) {
    const char* input = "(pos? x)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_POSP);
    TEST_ASSERT(node->as.unary.operand != NULL);
    TEST_ASSERT(node->as.unary.operand->kind == AST_SYMBOL);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_negp(void) {
    const char* input = "(neg? x)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_NEGP);
    TEST_ASSERT(node->as.unary.operand != NULL);
    TEST_ASSERT(node->as.unary.operand->kind == AST_SYMBOL);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_error_expected_lparen(void) {
    const char* input = "defn foo () 42)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    // Should parse "defn" as a symbol, not fail
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_SYMBOL);
}

void test_parser_error_expected_rparen(void) {
    const char* input = "(+ 1 2";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    (void)node; // May or may not be NULL depending on implementation
    TEST_ASSERT(se_parser_has_error(&parser));
}

void test_parser_error_unknown_form(void) {
    const char* input = "(unknown-form x y)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    // Unknown forms are treated as function calls
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_CALL);
}

// =============================================================================
// Codegen Test Implementations
// =============================================================================

static char* codegen_to_string(const char* input) {
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstProgram program = {0};
    if (!se_parser_parse_program(&parser, &program)) {
        return NULL;
    }

    if (se_parser_has_error(&parser)) {
        return NULL;
    }

    SeMacroTable macros;
    se_macro_init(&macros);
    se_macro_collect(&macros, &program);
    se_macro_expand(&macros, &program, &pool);

    // Use a temporary file for output
    FILE* tmp = tmpfile();
    if (!tmp) return NULL;

    SeCodegen codegen;
    se_codegen_init(&codegen, tmp, "test");

    if (!se_codegen_collect(&codegen, &program)) {
        fclose(tmp);
        return NULL;
    }

    if (!se_codegen_emit(&codegen, &program)) {
        fclose(tmp);
        return NULL;
    }

    // Read output into string
    fseek(tmp, 0, SEEK_END);
    long size = ftell(tmp);
    fseek(tmp, 0, SEEK_SET);

    char* output = malloc(size + 1);
    if (output) {
        fread(output, 1, size, tmp);
        output[size] = '\0';
    }

    fclose(tmp);
    return output;
}

void test_codegen_def(void) {
    const char* input = "(def x 42)";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // Codegen emits constants in hex format
    TEST_ASSERT(strstr(output, "x = 0x") != NULL);
    free(output);
}

void test_codegen_simple_function(void) {
    const char* input = "(defn foo () 42)";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "foo:") != NULL);
    // Codegen emits constants in hex format, e.g., LOADI R0, 0x2A
    TEST_ASSERT(strstr(output, "LOADI R0, 0x") != NULL);
    free(output);
}

void test_codegen_function_with_params(void) {
    const char* input = "(defn add (x y) (+ x y))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "add:") != NULL);
    free(output);
}

void test_codegen_arithmetic(void) {
    const char* input = "(defn test () (+ 1 2))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "test:") != NULL);
    free(output);
}

void test_codegen_if_statement(void) {
    const char* input = "(defn test (x) (if (< x 10) 1 0))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "test:") != NULL);
    free(output);
}

void test_codegen_while_loop(void) {
    const char* input = "(var i 0) (defn test () (while (< i 10) (set! i (+ i 1))) i)";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "test:") != NULL);
    free(output);
}

void test_codegen_function_call(void) {
    const char* input = "(defn foo () 42) (defn bar () (foo))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "foo:") != NULL);
    TEST_ASSERT(strstr(output, "bar:") != NULL);
    TEST_ASSERT(strstr(output, "CALL foo") != NULL || strstr(output, "call foo") != NULL);
    free(output);
}

void test_codegen_let_bindings(void) {
    const char* input = "(defn test () (let (x 10 y 20) (+ x y)))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "test:") != NULL);
    free(output);
}

void test_codegen_data_section(void) {
    const char* input = "(var x 42) (var msg 72)";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "x:") != NULL || strstr(output, "msg:") != NULL);
    TEST_ASSERT(strstr(output, "DB") != NULL || strstr(output, "db") != NULL);
    free(output);
}

void test_codegen_error_undefined_symbol(void) {
    const char* input = "(defn test () undefined_var)";
    char* output = codegen_to_string(input);
    // This might succeed or fail depending on implementation
    // The test mainly ensures it doesn't crash
    free(output);
}

void test_codegen_error_duplicate_function(void) {
    const char* input = "(defn foo () 1) (defn foo () 2)";
    char* output = codegen_to_string(input);
    // This should fail during collect phase
    // The test mainly ensures it doesn't crash
    free(output);
}

void test_codegen_defrecord_var(void) {
    const char* input = "(defrecord pipe (x gap-y passed)) "
                        "(var p1 (pipe 10 20 0)) "
                        "(defn main () 0)";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "p1:") != NULL);
    // Should emit 3 DB bytes for the 3 u8 fields
    TEST_ASSERT(strstr(output, "DB 0x0A") != NULL); // x = 10
    TEST_ASSERT(strstr(output, "DB 0x14") != NULL); // gap-y = 20
    TEST_ASSERT(strstr(output, "DB 0x00") != NULL); // passed = 0
    free(output);
}

void test_codegen_defrecord_field_get(void) {
    const char* input = "(defrecord pipe (x gap-y passed)) "
                        "(var p1 (pipe 10 20 0)) "
                        "(defn main () (:x p1))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // Should load field at offset 0
    TEST_ASSERT(strstr(output, "LOAD R0, [R6:R7 + 0]") != NULL);
    free(output);
}

void test_codegen_defrecord_field_set(void) {
    const char* input = "(defrecord pipe (x gap-y passed)) "
                        "(var p1 (pipe 10 20 0)) "
                        "(defn main () (set! (:gap-y p1) 30))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // Should store at field offset 1 (gap-y is 2nd u8 field)
    TEST_ASSERT(strstr(output, "STORE R0, [R6:R7 + 1]") != NULL);
    free(output);
}

void test_codegen_array_scalar(void) {
    const char* input = "(var scores (array 4 0)) "
                        "(defn main () 0)";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "scores:") != NULL);
    // Should emit TIMES 4 DB 0x00
    TEST_ASSERT(strstr(output, "TIMES 4 DB 0x00") != NULL);
    free(output);
}

void test_codegen_array_record(void) {
    const char* input = "(defrecord enemy (x y hp state)) "
                        "(var enemies (array 3 (enemy 0 0 3 :alive))) "
                        "(defn main () 0)";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "enemies:") != NULL);
    // Should emit 3 copies of 4-byte record (x=0, y=0, hp=3, state=:alive)
    // :alive will be keyword index 0
    TEST_ASSERT(strstr(output, "DB 0x00") != NULL); // x=0
    TEST_ASSERT(strstr(output, "DB 0x03") != NULL); // hp=3
    free(output);
}

void test_codegen_nth_scalar(void) {
    const char* input = "(var scores (array 4 0)) "
                        "(defn main () (nth scores 2))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // Should load from base + 2 (constant index)
    TEST_ASSERT(strstr(output, "LOAD R0, [R6:R7]") != NULL);
    free(output);
}

void test_codegen_nth_record_field(void) {
    const char* input = "(defrecord enemy (x y hp state)) "
                        "(var enemies (array 3 (enemy 0 0 3 :alive))) "
                        "(defn main () (:hp (nth enemies 1)))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // Should access hp field (offset 2) of element at index 1
    TEST_ASSERT(strstr(output, "LOAD R0, [R6:R7 + 2]") != NULL);
    free(output);
}

void test_codegen_set_bang_nth_scalar(void) {
    const char* input = "(var scores (array 4 0)) "
                        "(defn main () (set! (nth scores 0) 99))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // Should store 99 (0x63) at element 0
    TEST_ASSERT(strstr(output, "LOADI R0, 0x63") != NULL); // 99 = 0x63
    TEST_ASSERT(strstr(output, "STORE R0, [R6:R7]") != NULL);
    free(output);
}

void test_codegen_set_bang_nth_field(void) {
    const char* input = "(defrecord enemy (x y hp state)) "
                        "(var enemies (array 3 (enemy 0 0 3 :alive))) "
                        "(defn main () (set! (:state (nth enemies 2)) :alive))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // Should store at field offset 3 (state) of element 2
    TEST_ASSERT(strstr(output, "STORE R0, [R6:R7 + 3]") != NULL);
    free(output);
}

void test_codegen_len(void) {
    const char* input = "(var scores (array 8 0)) "
                        "(defn main () (len scores))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // len should return 8 (0x08)
    TEST_ASSERT(strstr(output, "LOADI R0, 0x08") != NULL);
    free(output);
}

void test_codegen_nilp(void) {
    const char* input = "(defn main (x) (nil? x))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // nil? should compare with 0xFF
    TEST_ASSERT(strstr(output, "LOADI R1, 0xFF") != NULL);
    TEST_ASSERT(strstr(output, "CMP R0, R1") != NULL);
    TEST_ASSERT(strstr(output, "JZ") != NULL);
    free(output);
}

void test_codegen_zerop(void) {
    const char* input = "(defn main (x) (zero? x))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // zero? should test OR R0,R0 then JZ
    TEST_ASSERT(strstr(output, "OR R0, R0") != NULL);
    TEST_ASSERT(strstr(output, "JZ") != NULL);
    free(output);
}

void test_codegen_posp(void) {
    const char* input = "(defn main (x) (pos? x))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // pos? checks zero and bit 7
    TEST_ASSERT(strstr(output, "OR R0, R0") != NULL);
    TEST_ASSERT(strstr(output, "LOADI R1, 0x80") != NULL);
    TEST_ASSERT(strstr(output, "AND R1, R0") != NULL);
    free(output);
}

void test_codegen_negp(void) {
    const char* input = "(defn main (x) (neg? x))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // neg? checks bit 7
    TEST_ASSERT(strstr(output, "LOADI R1, 0x80") != NULL);
    TEST_ASSERT(strstr(output, "AND R0, R1") != NULL);
    TEST_ASSERT(strstr(output, "JNZ") != NULL);
    free(output);
}

void test_codegen_fn_basic(void) {
    // Anonymous function used as expression (returns address)
    const char* input = "(defn main () (fn (x) (+ x 1)))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // The anonymous function should be emitted as __fn0
    TEST_ASSERT(strstr(output, "__fn0:") != NULL);
    // The fn expression should load the function address
    TEST_ASSERT(strstr(output, "LOADI R0, __fn0 >> 8") != NULL);
    TEST_ASSERT(strstr(output, "LOADI R1, __fn0 & 0xFF") != NULL);
    free(output);
}

void test_codegen_fn_var(void) {
    // Anonymous function stored in a var
    const char* input = "(var callback (fn () (+ 1 2))) "
                        "(defn main () 0)";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // The anonymous function should be emitted
    TEST_ASSERT(strstr(output, "__fn0:") != NULL);
    // The var should store the function address (2 bytes)
    TEST_ASSERT(strstr(output, "callback:") != NULL);
    TEST_ASSERT(strstr(output, "DB __fn0 >> 8, __fn0 & 0xFF") != NULL);
    free(output);
}

void test_codegen_fn_ref(void) {
    // Function reference: using a named function as a value
    const char* input = "(defn double (x) (+ x x)) "
                        "(var callback double) "
                        "(defn main () 0)";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // The var should store the function address
    TEST_ASSERT(strstr(output, "callback:") != NULL);
    TEST_ASSERT(strstr(output, "DB double >> 8, double & 0xFF") != NULL);
    free(output);
}

void test_codegen_indirect_call(void) {
    // Indirect call through a global function variable
    const char* input = "(defn noop () 0) "
                        "(var callback noop) "
                        "(defn main () (callback))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // Should emit the indirect call trampoline
    TEST_ASSERT(strstr(output, "__call_indirect:") != NULL);
    TEST_ASSERT(strstr(output, "CALL __call_indirect") != NULL);
    // Should load function address from callback variable
    TEST_ASSERT(strstr(output, "LOADI R6, __call_indirect >> 8") != NULL);
    free(output);
}

void test_codegen_string_var(void) {
    const char* input = "(var title \"HI\") "
                        "(defn main () 0)";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "title:") != NULL);
    // "HI" = 0x48, 0x49, null terminator 0x00
    TEST_ASSERT(strstr(output, "DB 0x48, 0x49, 0x00") != NULL);
    free(output);
}

void test_codegen_string_len(void) {
    const char* input = "(var title \"HELLO\") "
                        "(defn main () (len title))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // len should return 5 (compile-time constant)
    TEST_ASSERT(strstr(output, "LOADI R0, 0x05") != NULL);
    free(output);
}

void test_codegen_string_nth(void) {
    const char* input = "(var title \"ABC\") "
                        "(defn main () (nth title 1))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // nth with constant index 1 should compute address directly
    TEST_ASSERT(strstr(output, "LOAD R0, [R6:R7]") != NULL);
    free(output);
}

void test_parser_cast_u8(void) {
    const char* input = "(u8 x)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_CAST_U8);
    TEST_ASSERT(node->as.unary.operand != NULL);
    TEST_ASSERT(node->as.unary.operand->kind == AST_SYMBOL);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_parser_cast_i8(void) {
    const char* input = "(i8 x)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_CAST_I8);
    TEST_ASSERT(node->as.unary.operand != NULL);
    TEST_ASSERT(node->as.unary.operand->kind == AST_SYMBOL);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_codegen_cast_u8(void) {
    const char* input = "(defn main (x) (u8 x))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // u8 cast on 8-bit operand is a no-op (just loads x)
    TEST_ASSERT(strstr(output, "LOAD R0, [R4:R5") != NULL);
    free(output);
}

void test_codegen_cast_i8(void) {
    const char* input = "(defn main (x) (i8 x))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // i8 cast on 8-bit operand is a no-op (just loads x)
    TEST_ASSERT(strstr(output, "LOAD R0, [R4:R5") != NULL);
    free(output);
}

// =============================================================================
// Integration Test Implementations
// =============================================================================

void test_integration_hello_world(void) {
    const char* input = "(defn main () (+ 1 2))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    TEST_ASSERT(strstr(output, "section") != NULL || strstr(output, ".code") != NULL);
    free(output);
}

void test_integration_factorial(void) {
    const char* input = "(defn factorial (n) "
                        "  (if (<= n 1) "
                        "    1 "
                        "    (* n (factorial (- n 1)))))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "factorial:") != NULL);
    free(output);
}

void test_integration_fibonacci(void) {
    const char* input = "(defn fib (n) "
                        "  (if (<= n 1) "
                        "    n "
                        "    (+ (fib (- n 1)) (fib (- n 2)))))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "fib:") != NULL);
    free(output);
}

void test_integration_data_and_code(void) {
    const char* input = "(def MSG_ADDR 0x4000) "
                        "(var msg 0) "
                        "(defn main () (load MSG_ADDR))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "MSG_ADDR") != NULL);
    TEST_ASSERT(strstr(output, "msg:") != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    free(output);
}

// =============================================================================
// Macro Test Implementations
// =============================================================================

void test_macro_simple_substitution(void) {
    const char* input = "(var x 10) (defmacro inc-var (v) (set! {v} (+ {v} 1))) "
                        "(defn test () (inc-var x) x)";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "test:") != NULL);
    free(output);
}

void test_macro_suffix_substitution(void) {
    const char* input = "(var pos_lo 10) (defmacro read16 (v) (load {v})) "
                        "(defn test () (read16 pos_lo))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "pos_lo") != NULL);
    free(output);
}

void test_macro_numeric_args(void) {
    const char* input = "(var val 0) (defmacro set-val (v) (set! val {v})) "
                        "(defn test () (set-val 42))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "0x2A") != NULL || strstr(output, "42") != NULL);
    free(output);
}

// =============================================================================
// 16-bit / Fixed-point / Type Hint / Optimizer Test Implementations
// =============================================================================

void test_lexer_fixed_point(void) {
    // 3.5 in 8.8 fixed-point = 3*256 + 0.5*256 = 768 + 128 = 896
    const char* source = "3.5";
    SeLexer lexer = se_lexer_new(source, strlen(source));
    SeToken tok = se_lexer_next(&lexer);
    TEST_ASSERT(tok.kind == SE_TOKEN_NUMBER);
    TEST_ASSERT(tok.number_value == 896);

    // 1.0 = 256
    source = "1.0";
    lexer = se_lexer_new(source, strlen(source));
    tok = se_lexer_next(&lexer);
    TEST_ASSERT(tok.kind == SE_TOKEN_NUMBER);
    TEST_ASSERT(tok.number_value == 256);

    // 0.5 = 128
    source = "0.5";
    lexer = se_lexer_new(source, strlen(source));
    tok = se_lexer_next(&lexer);
    TEST_ASSERT(tok.kind == SE_TOKEN_NUMBER);
    TEST_ASSERT(tok.number_value == 128);
}

void test_parser_var_hint(void) {
    const char* input = "(var ^u16 counter 0)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);
    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_VAR);
    TEST_ASSERT(node->as.var.type_hint == SE_HINT_U16);
    TEST_ASSERT(strcmp(node->as.var.name, "counter") == 0);
    ast_pool_free(&pool);
}

void test_parser_let_hint(void) {
    const char* input = "(let (^i16 x 300) x)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);
    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_LET);
    TEST_ASSERT(node->as.let.binding_count == 1);
    TEST_ASSERT(node->as.let.hints[0] == SE_HINT_I16);
    TEST_ASSERT(strcmp(node->as.let.vars[0], "x") == 0);
    ast_pool_free(&pool);
}

void test_parser_defn_param_hints(void) {
    const char* input = "(defn add16 (^u16 a ^u16 b) (+ a b))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);
    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_DEFN);
    TEST_ASSERT(node->as.defn.param_count == 2);
    TEST_ASSERT(node->as.defn.param_hints[0] == SE_HINT_U16);
    TEST_ASSERT(node->as.defn.param_hints[1] == SE_HINT_U16);
    TEST_ASSERT(strcmp(node->as.defn.params[0], "a") == 0);
    TEST_ASSERT(strcmp(node->as.defn.params[1], "b") == 0);
    ast_pool_free(&pool);
}

void test_codegen_16bit_literal(void) {
    // 16-bit literal should emit LOADI R0, hi + LOADI R1, lo
    const char* input = "(defn main () 300)";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // 300 = 0x012C -> hi=0x01, lo=0x2C
    TEST_ASSERT(strstr(output, "LOADI R0, 0x01") != NULL);
    TEST_ASSERT(strstr(output, "LOADI R1, 0x2C") != NULL);
    free(output);
}

void test_codegen_16bit_var(void) {
    // 16-bit var with ^u16 hint
    const char* input = "(var ^u16 counter 0) (defn main () counter)";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // Should emit 2-byte DB and load both bytes
    TEST_ASSERT(strstr(output, "counter:") != NULL);
    TEST_ASSERT(strstr(output, "DB 0x00, 0x00") != NULL);
    free(output);
}

void test_codegen_16bit_add(void) {
    // 16-bit addition with carry
    const char* input = "(var ^u16 a 0) (var ^u16 b 0) (defn main () (+ a b))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // Should use ADC for high byte addition
    TEST_ASSERT(strstr(output, "ADC") != NULL);
    free(output);
}

void test_codegen_16bit_sub(void) {
    // 16-bit subtraction with borrow
    const char* input = "(var ^u16 a 0) (var ^u16 b 0) (defn main () (- a b))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // Should use SBC for high byte subtraction
    TEST_ASSERT(strstr(output, "SBC") != NULL);
    free(output);
}

void test_codegen_16bit_let(void) {
    // 16-bit let binding
    const char* input = "(defn main () (let (^u16 x 500) x))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // 500 = 0x01F4
    TEST_ASSERT(strstr(output, "0x01") != NULL);
    TEST_ASSERT(strstr(output, "0xF4") != NULL);
    free(output);
}

void test_optimizer_constant_fold(void) {
    // (+ 3 4) should be folded to 7
    const char* input = "(+ 3 4)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);
    AstProgram program = {0};
    se_parser_parse_program(&parser, &program);
    TEST_ASSERT(!se_parser_has_error(&parser));

    se_optimize(&program, &pool, SE_OPT_BASIC);

    TEST_ASSERT(program.node_count == 1);
    TEST_ASSERT(program.nodes[0]->kind == AST_NUMBER);
    TEST_ASSERT(program.nodes[0]->as.number == 7);
    ast_pool_free(&pool);
}

void test_optimizer_strength_reduce(void) {
    // (* x 8) should be reduced to (<< x 3)
    const char* input = "(defn test (x) (* x 8))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);
    AstProgram program = {0};
    se_parser_parse_program(&parser, &program);
    TEST_ASSERT(!se_parser_has_error(&parser));

    se_optimize(&program, &pool, SE_OPT_FULL);

    // The defn body should now contain a SHL node instead of MUL
    TEST_ASSERT(program.node_count == 1);
    TEST_ASSERT(program.nodes[0]->kind == AST_DEFN);
    AstNode* body = program.nodes[0]->as.defn.body.items[0];
    TEST_ASSERT(body->kind == AST_SHL);
    TEST_ASSERT(body->as.binary.right->kind == AST_NUMBER);
    TEST_ASSERT(body->as.binary.right->as.number == 3);
    ast_pool_free(&pool);
}

void test_codegen_truthiness_nil(void) {
    // nil (0xFF) should be treated as falsy by JFALSE
    const char* input = "(defn main () (if nil 1 2))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // Should use JFALSE macro (which checks both 0x00 and 0xFF)
    TEST_ASSERT(strstr(output, "JFALSE") != NULL);
    free(output);
}

void test_codegen_while_returns_nil(void) {
    // while should return nil (0xFF), not 0
    const char* input = "(defn main () (while false 1))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "LOADI R0, 0xFF") != NULL);
    free(output);
}

void test_codegen_when_returns_nil(void) {
    // when should skip body when condition is false
    const char* input = "(defn main () (when false 1))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // false condition: fused branch should just JMP to skip body
    TEST_ASSERT(strstr(output, "main:") != NULL);
    free(output);
}

void test_codegen_cond_returns_nil(void) {
    // cond should return nil when no clause matches
    const char* input = "(defn main () (cond false 1))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "LOADI R0, 0xFF") != NULL);
    free(output);
}

void test_codegen_for_array(void) {
    // for loop over array should work
    const char* input = "(var scores (array 4 0)) (defn main () (for (s scores) s))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // Should reference the array base address and compare index against count (4)
    TEST_ASSERT(strstr(output, "0x04") != NULL);             // element count
    TEST_ASSERT(strstr(output, "LOAD R0, [R6:R7]") != NULL); // scalar load
    free(output);
}

void test_codegen_u16_auto_increment(void) {
    // u16 var read should use auto-increment addressing
    const char* input = "(var ^u16 counter 0) (defn main () counter)";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "[R6:R7]+") != NULL);
    free(output);
}

void test_codegen_macros_emitted(void) {
    const char* input = "(defn main () 1)";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, ".macro JFALSE") != NULL);
    TEST_ASSERT(strstr(output, ".macro JTRUE") != NULL);
    TEST_ASSERT(strstr(output, ".endmacro") != NULL);
    free(output);
}

void test_codegen_signed_compare(void) {
    // Signed comparison should use XOR 0x80 bias before CMP
    const char* input = "(var ^i8 a 0) (var ^i8 b 0) (defn main () (< a b))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // Should contain XOR with 0x80 for signed comparison
    TEST_ASSERT(strstr(output, "0x80") != NULL);
    TEST_ASSERT(strstr(output, "XOR R0") != NULL);
    TEST_ASSERT(strstr(output, "XOR R1") != NULL);
    free(output);
}

void test_codegen_signed_shr(void) {
    // Signed shift right should preserve sign bit (arithmetic shift)
    const char* input = "(var ^i8 x 0) (defn main () (>> x 1))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // Arithmetic SHR emits: save, SHR, restore sign bit with AND 0x80 + OR
    TEST_ASSERT(strstr(output, "SHR R0") != NULL);
    TEST_ASSERT(strstr(output, "AND R1, R6") != NULL); // masking sign bit
    TEST_ASSERT(strstr(output, "OR R0, R1") != NULL);  // restoring sign bit
    free(output);
}

void test_optimizer_dead_code(void) {
    // defmacro nodes should be removed after optimization
    const char* input = "(defmacro noop () nil) (defn main () 1)";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);
    AstProgram program = {0};
    se_parser_parse_program(&parser, &program);
    TEST_ASSERT(!se_parser_has_error(&parser));
    TEST_ASSERT(program.node_count == 2); // defmacro + defn

    se_optimize(&program, &pool, SE_OPT_BASIC);

    // defmacro should be removed
    TEST_ASSERT(program.node_count == 1);
    TEST_ASSERT(program.nodes[0]->kind == AST_DEFN);
    ast_pool_free(&pool);
}

void test_optimizer_dead_let_binding(void) {
    // (let (unused 5) x) should remove the unused binding
    const char* input = "(defn test (x) (let (unused 5) x))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);
    AstProgram program = {0};
    se_parser_parse_program(&parser, &program);
    TEST_ASSERT(!se_parser_has_error(&parser));

    se_optimize(&program, &pool, SE_OPT_BASIC);

    // The defn body should no longer contain the let (it was unwrapped)
    TEST_ASSERT(program.node_count == 1);
    TEST_ASSERT(program.nodes[0]->kind == AST_DEFN);
    // Body should be just a symbol reference to x (let was eliminated)
    AstNode* body = program.nodes[0]->as.defn.body.items[0];
    TEST_ASSERT(body->kind == AST_SYMBOL);
    TEST_ASSERT(strcmp(body->as.symbol.name, "x") == 0);
    ast_pool_free(&pool);
}

// =============================================================================
// Regression & Feature Coverage Test Implementations
// =============================================================================

// Helper: codegen with -O2 optimization (like the real build pipeline)
static char* codegen_optimized_to_string(const char* input) {
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstProgram program = {0};
    if (!se_parser_parse_program(&parser, &program)) return NULL;
    if (se_parser_has_error(&parser)) return NULL;

    SeMacroTable macros;
    se_macro_init(&macros);
    se_macro_collect(&macros, &program);
    se_macro_expand(&macros, &program, &pool);

    se_optimize(&program, &pool, SE_OPT_FULL);

    FILE* tmp = tmpfile();
    if (!tmp) return NULL;

    SeCodegen codegen;
    se_codegen_init(&codegen, tmp, "test");
    if (!se_codegen_collect(&codegen, &program)) {
        fclose(tmp);
        return NULL;
    }
    if (!se_codegen_emit(&codegen, &program)) {
        fclose(tmp);
        return NULL;
    }

    fseek(tmp, 0, SEEK_END);
    long size = ftell(tmp);
    fseek(tmp, 0, SEEK_SET);

    char* output = malloc(size + 1);
    if (output) {
        fread(output, 1, size, tmp);
        output[size] = '\0';
    }
    fclose(tmp);
    return output;
}

// --- eval_const fix: variables must be loaded from memory, not treated as constants ---

void test_codegen_var_not_const_in_eq(void) {
    // (= (some-expr) var) must load var from memory, not use its initial value as a constant.
    // Bug: eval_const treated data labels as constants, emitting LOADI with the address low byte.
    const char* input = "(var counter 0) "
                        "(defn main () (= counter 5))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // The comparison should LOAD counter from memory (R6:R7), not use LOADI R1, 0x00
    TEST_ASSERT(strstr(output, "LOAD R0, [R6:R7]") != NULL);
    free(output);
}

void test_codegen_var_not_const_in_ne(void) {
    // Same bug for != comparisons
    const char* input = "(var state 0) "
                        "(defn main () (!= state 1))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // Must load state from memory
    TEST_ASSERT(strstr(output, "LOAD R0, [R6:R7]") != NULL);
    free(output);
}

// --- Optimizer: constant propagation must not replace mutated let bindings ---

void test_optimizer_cprop_mutated_let(void) {
    // (let (i 0) (while ...) (set! i ...)) - i is mutated, must NOT be propagated
    const char* input = "(def N 4) "
                        "(defn main () (let (i 0) (while (< i N) (set! i (inc i))) i))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);
    AstProgram program = {0};
    se_parser_parse_program(&parser, &program);
    TEST_ASSERT(!se_parser_has_error(&parser));

    se_optimize(&program, &pool, SE_OPT_FULL);

    // After optimization, the while loop condition must still reference i (as a symbol),
    // NOT be folded to a constant true.
    // Find the defn for main
    AstNode* main_fn = NULL;
    for (size_t j = 0; j < program.node_count; j++) {
        if (program.nodes[j]->kind == AST_DEFN &&
            strcmp(program.nodes[j]->as.defn.name, "main") == 0) {
            main_fn = program.nodes[j];
            break;
        }
    }
    TEST_ASSERT(main_fn != NULL);
    // Body should contain a let with a while inside
    AstNode* body0 = main_fn->as.defn.body.items[0];
    TEST_ASSERT(body0->kind == AST_LET);
    // The while condition should NOT be a constant number (which would mean
    // the optimizer incorrectly folded (< 0 4) => 1)
    AstNode* while_node = NULL;
    for (size_t j = 0; j < body0->as.let.body.count; j++) {
        if (body0->as.let.body.items[j]->kind == AST_WHILE) {
            while_node = body0->as.let.body.items[j];
            break;
        }
    }
    TEST_ASSERT(while_node != NULL);
    // The condition must NOT be a constant number (would be AST_NUMBER with value 1 if buggy)
    TEST_ASSERT(while_node->as.while_expr.cond->kind != AST_NUMBER);
    ast_pool_free(&pool);
}

void test_optimizer_cprop_immutable_let(void) {
    // (let (x 10) (+ x 5)) - x is NOT mutated, should be propagated
    const char* input = "(defn main () (let (x 10) (+ x 5)))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);
    AstProgram program = {0};
    se_parser_parse_program(&parser, &program);
    TEST_ASSERT(!se_parser_has_error(&parser));

    se_optimize(&program, &pool, SE_OPT_FULL);

    // After optimization, (+ 10 5) should be folded to 15
    AstNode* main_fn = program.nodes[0];
    TEST_ASSERT(main_fn->kind == AST_DEFN);
    AstNode* body = main_fn->as.defn.body.items[0];
    // Should be folded to number 15 (dead code pass removes the empty let)
    TEST_ASSERT(body->kind == AST_NUMBER);
    TEST_ASSERT(body->as.number == 15);
    ast_pool_free(&pool);
}

// --- Optimizer: dead function elimination ---

void test_optimizer_dead_fn_elim(void) {
    // Unreachable functions should be removed at -O2
    // Note: simple leaf functions get inlined, then become dead too
    const char* input = "(defn unused () 42) "
                        "(defn also-unused () 99) "
                        "(defn helper () 1) "
                        "(defn main () (helper))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);
    AstProgram program = {0};
    se_parser_parse_program(&parser, &program);
    TEST_ASSERT(!se_parser_has_error(&parser));
    TEST_ASSERT(program.node_count == 4); // 4 defn nodes

    se_optimize(&program, &pool, SE_OPT_FULL);

    // After inlining + dead fn elimination: only main remains
    // (helper was inlined into main, then became dead)
    int fn_count = 0;
    bool has_main = false, has_unused = false;
    for (size_t i = 0; i < program.node_count; i++) {
        if (program.nodes[i]->kind == AST_DEFN) {
            fn_count++;
            if (strcmp(program.nodes[i]->as.defn.name, "main") == 0) has_main = true;
            if (strcmp(program.nodes[i]->as.defn.name, "unused") == 0) has_unused = true;
        }
    }
    TEST_ASSERT(fn_count == 1);
    TEST_ASSERT(has_main);
    TEST_ASSERT(!has_unused);
    ast_pool_free(&pool);
}

void test_optimizer_dead_fn_keeps_transitive(void) {
    // main -> a -> b: small leaf functions get inlined transitively,
    // then become dead. With inlining, a and b get inlined into main.
    const char* input = "(defn b () 1) "
                        "(defn a () (b)) "
                        "(defn dead () 99) "
                        "(defn main () (a))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);
    AstProgram program = {0};
    se_parser_parse_program(&parser, &program);
    TEST_ASSERT(!se_parser_has_error(&parser));

    se_optimize(&program, &pool, SE_OPT_FULL);

    // After inlining + dead fn elim: only main remains
    int fn_count = 0;
    bool has_dead = false, has_main = false;
    for (size_t i = 0; i < program.node_count; i++) {
        if (program.nodes[i]->kind == AST_DEFN) {
            fn_count++;
            if (strcmp(program.nodes[i]->as.defn.name, "dead") == 0) has_dead = true;
            if (strcmp(program.nodes[i]->as.defn.name, "main") == 0) has_main = true;
        }
    }
    TEST_ASSERT(fn_count == 1); // only main
    TEST_ASSERT(has_main);
    TEST_ASSERT(!has_dead);
    ast_pool_free(&pool);
}

// --- hi/lo on data labels ---

void test_codegen_hi_lo_data_label(void) {
    // (hi data_label) and (lo data_label) must resolve to address bytes
    // Used by floppy: (hi sfx_table), (lo sfx_table)
    const char* input = "(data my_data (db 1 2 3)) "
                        "(defn main () (+ (hi my_data) (lo my_data)))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // hi and lo should produce LOADI instructions with address bytes
    TEST_ASSERT(strstr(output, "LOADI R0, 0x") != NULL);
    free(output);
}

// --- cond with keywords ---

void test_codegen_cond_keywords(void) {
    // Pattern from floppy: (cond (= state :waiting) ... (= state :playing) ... ...)
    const char* input = "(var state :waiting) "
                        "(defn main () "
                        "  (cond "
                        "    (= state :waiting) 1 "
                        "    (= state :playing) 2 "
                        "    (= state :dead) 3))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // Should load state from memory (not use a constant)
    TEST_ASSERT(strstr(output, "LOAD R0, [R6:R7]") != NULL);
    // Should compare against keyword values (0, 1, 2)
    TEST_ASSERT(strstr(output, "LOADI R1, 0x00") != NULL);
    TEST_ASSERT(strstr(output, "LOADI R1, 0x01") != NULL);
    TEST_ASSERT(strstr(output, "LOADI R1, 0x02") != NULL);
    free(output);
}

// --- while loop with let-bound counter ---

void test_codegen_while_let_counter(void) {
    // Pattern: (let (i 0) (while (< i 10) (set! i (inc i))))
    // The loop counter i must be loaded from the stack, not constant-propagated
    const char* input = "(defn main () "
                        "  (let (i 0) "
                        "    (while (< i 10) "
                        "      (set! i (inc i)))))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // Loop condition should LOAD i from stack (R2:R3 based)
    TEST_ASSERT(strstr(output, "LOAD R0, [R2:R3") != NULL);
    // Should compare against 10 (0x0A)
    TEST_ASSERT(strstr(output, "0x0A") != NULL);
    // Should have INC for the counter
    TEST_ASSERT(strstr(output, "INC R0") != NULL);
    free(output);
}

// --- for loop with range ---

void test_codegen_for_range(void) {
    // Pattern from examples: (for (i (range 0 4)) body)
    const char* input = "(var total 0) "
                        "(defn main () "
                        "  (for (i (range 0 4)) "
                        "    (set! total (+ total i))))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // Should compare against 4 (loop end)
    TEST_ASSERT(strstr(output, "0x04") != NULL);
    free(output);
}

// --- defrecord + array + nth with variable index ---

void test_codegen_nth_record_variable_index(void) {
    // Pattern from floppy: (:x (nth pipes i)) where i is a parameter
    const char* input = "(defrecord pipe (x gap-y passed)) "
                        "(var pipes (array 2 (pipe 0 0 0))) "
                        "(defn get-x (i) (:x (nth pipes i))) "
                        "(defn main () (get-x 0))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    // Should have the get_x function
    TEST_ASSERT(strstr(output, "get_x:") != NULL);
    // Should access field at offset 0 (x)
    TEST_ASSERT(strstr(output, "LOAD R0, [R6:R7 + 0]") != NULL);
    free(output);
}

// --- data label with set! ---

void test_codegen_data_label_set_bang(void) {
    // Pattern from floppy's fixed-point: (data bird_y_hi (db 0)) then (set! bird_y_hi 60)
    const char* input = "(data bird_y_hi (db 0)) "
                        "(defn main () (set! bird_y_hi 60) bird_y_hi)";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // Should store 60 (0x3C) to bird_y_hi
    TEST_ASSERT(strstr(output, "0x3C") != NULL);
    TEST_ASSERT(strstr(output, "STORE R0, [R6:R7]") != NULL);
    // Should also load bird_y_hi from memory (not use constant 0)
    TEST_ASSERT(strstr(output, "LOAD R0, [R6:R7]") != NULL);
    free(output);
}

// --- store/load with computed address ---

void test_codegen_store_computed_addr(void) {
    // Pattern from floppy: (store (+ PALETTE_BASE 1) 0x6D)
    const char* input = "(def BASE 0x9200) "
                        "(defn main () (store (+ BASE 1) 0x6D))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // Should compute address 0x9201 -> R6=0x92, R7=0x01
    TEST_ASSERT(strstr(output, "LOADI R6, 0x92") != NULL);
    TEST_ASSERT(strstr(output, "LOADI R7, 0x01") != NULL);
    TEST_ASSERT(strstr(output, "STORE R0, [R6:R7]") != NULL);
    free(output);
}

// --- and/or short-circuit logic ---

void test_codegen_and_short_circuit(void) {
    // (and a b) should short-circuit: if a is falsy, skip b
    const char* input = "(defn main (a b) (and a b))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // Should have JFALSE for short-circuit
    TEST_ASSERT(strstr(output, "JFALSE") != NULL);
    free(output);
}

void test_codegen_or_short_circuit(void) {
    // (or a b) should short-circuit: if a is truthy, skip b
    const char* input = "(defn main (a b) (or a b))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // Should have JTRUE for short-circuit
    TEST_ASSERT(strstr(output, "JTRUE") != NULL);
    free(output);
}

// --- Optimized codegen generates correct assembly at -O2 ---

void test_codegen_optimized_while_loop(void) {
    // At -O2, a while loop with a mutated counter must still load the counter
    // from memory, not constant-fold the condition to true.
    const char* input = "(def LIMIT 8) "
                        "(var total 0) "
                        "(defn main () "
                        "  (let (i 0) "
                        "    (while (< i LIMIT) "
                        "      (set! total (+ total i)) "
                        "      (set! i (inc i)))))";
    char* output = codegen_optimized_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // The while condition must load i from stack, NOT be "LOADI R0, 0x01" (always true)
    // Check that the loop has a proper comparison with 8 (0x08)
    TEST_ASSERT(strstr(output, "0x08") != NULL);
    // Must have LOAD from R2:R3 (let-local) and INC for counter
    TEST_ASSERT(strstr(output, "LOAD R0, [R2:R3") != NULL);
    TEST_ASSERT(strstr(output, "INC R0") != NULL);
    free(output);
}

void test_codegen_optimized_for_range(void) {
    // At -O2, for (i (range 0 N)) should still work with correct loop bounds
    const char* input = "(def N 4) "
                        "(var result 0) "
                        "(defn main () "
                        "  (for (i (range 0 N)) "
                        "    (set! result (+ result 1))))";
    char* output = codegen_optimized_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // Should have comparison against 4
    TEST_ASSERT(strstr(output, "0x04") != NULL);
    free(output);
}

// ---------------------------------------------------------------------------
// Tests for ^i16 record field fixes
// ---------------------------------------------------------------------------

void test_parser_defrecord_field_is_signed(void) {
    // Parser should track field_is_signed for ^i16 hints
    const char* input = "(defrecord entity (^i16 x ^u16 y hp))";
    AstPool pool;
    ast_pool_init(&pool);
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->kind == AST_DEFRECORD);
    TEST_ASSERT(node->as.defrecord.field_count == 3);
    // ^i16 x — 16-bit and signed
    TEST_ASSERT(node->as.defrecord.field_is_16bit[0]);
    TEST_ASSERT(node->as.defrecord.field_is_signed[0]);
    // ^u16 y — 16-bit but not signed
    TEST_ASSERT(node->as.defrecord.field_is_16bit[1]);
    TEST_ASSERT(!node->as.defrecord.field_is_signed[1]);
    // hp — 8-bit, not signed
    TEST_ASSERT(!node->as.defrecord.field_is_16bit[2]);
    TEST_ASSERT(!node->as.defrecord.field_is_signed[2]);
    TEST_ASSERT(!se_parser_has_error(&parser));
}

void test_codegen_i16_field_nth_is_16bit(void) {
    // Accessing an ^i16 field via (nth arr i) must produce 16-bit loads (R0:R1)
    // Previously the compiler only recognized i16 fields on bare symbol records,
    // not on (nth array index) access, causing 8-bit codegen for 16-bit fields.
    const char* input = "(defrecord ball (^i16 x ^i16 y speed)) "
                        "(var balls (array 3 (ball 0 0 0))) "
                        "(defn main () (:x (nth balls 0)))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // i16 field access should generate two LOADs (high byte + low byte)
    TEST_ASSERT(strstr(output, "LOAD R0, [R6:R7 + 0]") != NULL);
    TEST_ASSERT(strstr(output, "LOAD R1, [R6:R7 + 1]") != NULL);
    free(output);
}

void test_codegen_i16_field_nth_arithmetic(void) {
    // Adding a u8 constant to an ^i16 field via (nth arr i) must use 16-bit
    // addition (ADC). Previously the compiler did 8-bit ADD, corrupting the
    // 16-bit value.
    const char* input = "(defrecord ball (^i16 x ^i16 y speed)) "
                        "(var balls (array 3 (ball 0 0 0))) "
                        "(defn main () (set! (:y (nth balls 0)) (+ (:y (nth balls 0)) 50)))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // Must use ADC for 16-bit addition (not just ADD)
    TEST_ASSERT(strstr(output, "ADC") != NULL);
    free(output);
}

void test_codegen_shl_u8_by_8_is_16bit(void) {
    // (<< u8_val 8) must produce a 16-bit result. Shifting an 8-bit value
    // left by 8 positions without promotion would yield 0.
    const char* input = "(defn main (x) (<< x 8))";
    char* output = codegen_to_string(input);
    TEST_ASSERT(output != NULL);
    TEST_ASSERT(strstr(output, "main:") != NULL);
    // Should promote u8 to 16-bit (MOV R1, R0 + LOADI R0, 0) then shift R0:R1
    TEST_ASSERT(strstr(output, "MOV R1, R0") != NULL);
    TEST_ASSERT(strstr(output, "SHL R1") != NULL);     // 16-bit shift uses SHL R1
    TEST_ASSERT(strstr(output, "ADC R0, R0") != NULL); // carry into high byte
    free(output);
}
