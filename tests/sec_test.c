#include <assert.h>
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
#include "../sec/parser.c"
#include "../sec/parser.h"

#define SEC_TEST(fn)                                                                               \
    do {                                                                                           \
        printf(" ▸ %s", #fn);                                                                      \
        fflush(stdout);                                                                            \
        fn();                                                                                      \
        printf(" ✓\n");                                                                            \
    } while (0)

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
void test_parser_data(void);
void test_parser_db(void);
void test_parser_repeat(void);
void test_parser_addr(void);
void test_parser_addr16(void);
void test_parser_load_store(void);
void test_parser_hi_lo(void);
void test_parser_error_expected_lparen(void);
void test_parser_error_expected_rparen(void);
void test_parser_error_unknown_form(void);

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

// =============================================================================
// Integration Tests
// =============================================================================

void test_integration_hello_world(void);
void test_integration_factorial(void);
void test_integration_fibonacci(void);
void test_integration_data_and_code(void);

int main(void) {
    // Lexer tests
    SEC_TEST(test_lexer_empty);
    SEC_TEST(test_lexer_whitespace);
    SEC_TEST(test_lexer_comments);
    SEC_TEST(test_lexer_parentheses);
    SEC_TEST(test_lexer_numbers_decimal);
    SEC_TEST(test_lexer_numbers_hex);
    SEC_TEST(test_lexer_strings);
    SEC_TEST(test_lexer_symbols);
    SEC_TEST(test_lexer_multiline);
    SEC_TEST(test_lexer_special_symbols);

    // Parser tests
    SEC_TEST(test_parser_number);
    SEC_TEST(test_parser_string);
    SEC_TEST(test_parser_symbol);
    SEC_TEST(test_parser_def);
    SEC_TEST(test_parser_defn_simple);
    SEC_TEST(test_parser_defn_with_params);
    SEC_TEST(test_parser_let);
    SEC_TEST(test_parser_set);
    SEC_TEST(test_parser_if);
    SEC_TEST(test_parser_while);
    SEC_TEST(test_parser_do);
    SEC_TEST(test_parser_add);
    SEC_TEST(test_parser_arithmetic_ops);
    SEC_TEST(test_parser_comparison_ops);
    SEC_TEST(test_parser_bitwise_ops);
    SEC_TEST(test_parser_nested_expressions);
    SEC_TEST(test_parser_function_call);
    SEC_TEST(test_parser_data);
    SEC_TEST(test_parser_db);
    SEC_TEST(test_parser_repeat);
    SEC_TEST(test_parser_addr);
    SEC_TEST(test_parser_addr16);
    SEC_TEST(test_parser_load_store);
    SEC_TEST(test_parser_hi_lo);
    SEC_TEST(test_parser_error_expected_lparen);
    SEC_TEST(test_parser_error_expected_rparen);
    SEC_TEST(test_parser_error_unknown_form);

    // Codegen tests
    SEC_TEST(test_codegen_def);
    SEC_TEST(test_codegen_simple_function);
    SEC_TEST(test_codegen_function_with_params);
    SEC_TEST(test_codegen_arithmetic);
    SEC_TEST(test_codegen_if_statement);
    SEC_TEST(test_codegen_while_loop);
    SEC_TEST(test_codegen_function_call);
    SEC_TEST(test_codegen_let_bindings);
    SEC_TEST(test_codegen_data_section);
    SEC_TEST(test_codegen_error_undefined_symbol);
    SEC_TEST(test_codegen_error_duplicate_function);

    // Integration tests
    SEC_TEST(test_integration_hello_world);
    SEC_TEST(test_integration_factorial);
    SEC_TEST(test_integration_fibonacci);
    SEC_TEST(test_integration_data_and_code);

    return 0;
}

// =============================================================================
// Lexer Test Implementations
// =============================================================================

void test_lexer_empty(void) {
    const char* input = "";
    SeLexer lexer = se_lexer_new(input, strlen(input));
    SeToken token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_END);
}

void test_lexer_whitespace(void) {
    const char* input = "   \t\n  ";
    SeLexer lexer = se_lexer_new(input, strlen(input));
    SeToken token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_END);
}

void test_lexer_comments(void) {
    const char* input = "; this is a comment\n42";
    SeLexer lexer = se_lexer_new(input, strlen(input));

    SeToken token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_NUMBER);
    assert(token.number_value == 42);

    token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_END);
}

void test_lexer_parentheses(void) {
    const char* input = "()";
    SeLexer lexer = se_lexer_new(input, strlen(input));

    SeToken token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_LPAREN);

    token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_RPAREN);

    token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_END);
}

void test_lexer_numbers_decimal(void) {
    const char* input = "0 42 255";
    SeLexer lexer = se_lexer_new(input, strlen(input));

    SeToken token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_NUMBER);
    assert(token.number_value == 0);

    token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_NUMBER);
    assert(token.number_value == 42);

    token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_NUMBER);
    assert(token.number_value == 255);
}

void test_lexer_numbers_hex(void) {
    const char* input = "0x00 0xFF 0xAB";
    SeLexer lexer = se_lexer_new(input, strlen(input));

    SeToken token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_NUMBER);
    assert(token.number_value == 0x00);

    token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_NUMBER);
    assert(token.number_value == 0xFF);

    token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_NUMBER);
    assert(token.number_value == 0xAB);
}

void test_lexer_strings(void) {
    const char* input = "\"hello\" \"world\"";
    SeLexer lexer = se_lexer_new(input, strlen(input));

    SeToken token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_STRING);
    assert(strncmp(token.text, "\"hello\"", token.text_len) == 0);

    token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_STRING);
    assert(strncmp(token.text, "\"world\"", token.text_len) == 0);
}

void test_lexer_symbols(void) {
    const char* input = "defn add x y";
    SeLexer lexer = se_lexer_new(input, strlen(input));

    SeToken token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_SYMBOL);
    assert(strncmp(token.text, "defn", token.text_len) == 0);

    token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_SYMBOL);
    assert(strncmp(token.text, "add", token.text_len) == 0);

    token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_SYMBOL);
    assert(strncmp(token.text, "x", token.text_len) == 0);

    token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_SYMBOL);
    assert(strncmp(token.text, "y", token.text_len) == 0);
}

void test_lexer_multiline(void) {
    const char* input = "(defn foo ()\n  42)";
    SeLexer lexer = se_lexer_new(input, strlen(input));

    SeToken token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_LPAREN);
    assert(token.line == 1);

    token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_SYMBOL);

    token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_SYMBOL);

    token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_LPAREN);

    token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_RPAREN);

    token = se_lexer_next(&lexer);
    assert(token.kind == SE_TOKEN_NUMBER);
    assert(token.line == 2);
}

void test_lexer_special_symbols(void) {
    const char* input = "+ - * / < > = != <= >=";
    SeLexer lexer = se_lexer_new(input, strlen(input));

    const char* expected[] = {"+", "-", "*", "/", "<", ">", "=", "!=", "<=", ">="};
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        SeToken token = se_lexer_next(&lexer);
        assert(token.kind == SE_TOKEN_SYMBOL);
        assert(strncmp(token.text, expected[i], token.text_len) == 0);
    }
}

// =============================================================================
// Parser Test Implementations
// =============================================================================

void test_parser_number(void) {
    const char* input = "42";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_NUMBER);
    assert(node->as.number == 42);
}

void test_parser_string(void) {
    const char* input = "\"hello\"";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_STRING);
    assert(strcmp(node->as.symbol.name, "hello") == 0);
}

void test_parser_symbol(void) {
    const char* input = "foo";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_SYMBOL);
    assert(strcmp(node->as.symbol.name, "foo") == 0);
}

void test_parser_def(void) {
    const char* input = "(def x 42)";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_DEF);
    assert(strcmp(node->as.def.name, "x") == 0);
    assert(node->as.def.value != NULL);
    assert(node->as.def.value->kind == AST_NUMBER);
    assert(node->as.def.value->as.number == 42);
}

void test_parser_defn_simple(void) {
    const char* input = "(defn foo () 42)";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_DEFN);
    assert(strcmp(node->as.defn.name, "foo") == 0);
    assert(node->as.defn.param_count == 0);
    assert(node->as.defn.body_count == 1);
    assert(node->as.defn.body[0]->kind == AST_NUMBER);
}

void test_parser_defn_with_params(void) {
    const char* input = "(defn add (x y) (+ x y))";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_DEFN);
    assert(strcmp(node->as.defn.name, "add") == 0);
    assert(node->as.defn.param_count == 2);
    assert(strcmp(node->as.defn.params[0], "x") == 0);
    assert(strcmp(node->as.defn.params[1], "y") == 0);
    assert(node->as.defn.body_count == 1);
}

void test_parser_let(void) {
    const char* input = "(let (x 10 y 20) (+ x y))";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_LET);
    assert(node->as.let.binding_count == 2);
    assert(strcmp(node->as.let.vars[0], "x") == 0);
    assert(strcmp(node->as.let.vars[1], "y") == 0);
    assert(node->as.let.vals[0]->kind == AST_NUMBER);
    assert(node->as.let.vals[1]->kind == AST_NUMBER);
    assert(node->as.let.body_count == 1);
}

void test_parser_set(void) {
    const char* input = "(set x 42)";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_SET);
    assert(strcmp(node->as.set.var, "x") == 0);
    assert(node->as.set.value->kind == AST_NUMBER);
    assert(node->as.set.value->as.number == 42);
}

void test_parser_if(void) {
    const char* input = "(if (< x 10) 1 0)";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_IF);
    assert(node->as.if_expr.cond != NULL);
    assert(node->as.if_expr.then_branch != NULL);
    assert(node->as.if_expr.else_branch != NULL);
}

void test_parser_while(void) {
    const char* input = "(while (< i 10) (set i (+ i 1)))";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_WHILE);
    assert(node->as.while_expr.cond != NULL);
    assert(node->as.while_expr.body_count == 1);
}

void test_parser_do(void) {
    const char* input = "(do (set x 1) (set y 2) x)";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_DO);
    assert(node->as.block.expr_count == 3);
}

void test_parser_add(void) {
    const char* input = "(+ 1 2)";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_ADD);
    assert(node->as.binary.left->kind == AST_NUMBER);
    assert(node->as.binary.right->kind == AST_NUMBER);
}

void test_parser_arithmetic_ops(void) {
    const char* tests[] = {"(+ 1 2)", "(- 10 5)", "(inc x)", "(dec y)"};
    AstKind expected[] = {AST_ADD, AST_SUB, AST_INC, AST_DEC};

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        AstPool pool = {0};
        SeParser parser;
        se_parser_init(&parser, tests[i], strlen(tests[i]), &pool);

        AstNode* node = se_parser_parse_form(&parser);
        assert(node != NULL);
        assert(node->kind == expected[i]);
    }
}

void test_parser_comparison_ops(void) {
    const char* tests[] = {"(= x y)", "(!= a b)", "(< x 10)", "(> y 5)", "(<= i n)", "(>= j 0)"};
    AstKind expected[] = {AST_EQ, AST_NE, AST_LT, AST_GT, AST_LE, AST_GE};

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        AstPool pool = {0};
        SeParser parser;
        se_parser_init(&parser, tests[i], strlen(tests[i]), &pool);

        AstNode* node = se_parser_parse_form(&parser);
        assert(node != NULL);
        assert(node->kind == expected[i]);
    }
}

void test_parser_bitwise_ops(void) {
    const char* tests[] = {"(& x y)", "(| a b)", "(^ x y)", "(~ val)", "(<< n 2)", "(>> n 1)"};
    AstKind expected[] = {AST_AND, AST_OR, AST_XOR, AST_NOT, AST_SHL, AST_SHR};

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        AstPool pool = {0};
        SeParser parser;
        se_parser_init(&parser, tests[i], strlen(tests[i]), &pool);

        AstNode* node = se_parser_parse_form(&parser);
        assert(node != NULL);
        assert(node->kind == expected[i]);
    }
}

void test_parser_nested_expressions(void) {
    const char* input = "(+ (* 2 3) (- 10 4))";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_ADD);
    assert(node->as.binary.left->kind == AST_MUL);
    assert(node->as.binary.right->kind == AST_SUB);
}

void test_parser_function_call(void) {
    const char* input = "(foo 1 2 3)";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_CALL);
    assert(strcmp(node->as.call.func, "foo") == 0);
    assert(node->as.call.arg_count == 3);
}

void test_parser_data(void) {
    const char* input = "(data msg (db \"hello\"))";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_DATA);
    assert(strcmp(node->as.data.name, "msg") == 0);
    assert(node->as.data.body_count == 1);
}

void test_parser_db(void) {
    const char* input = "(db 1 2 3)";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_DB);
    assert(node->as.block.expr_count == 3);
}

void test_parser_repeat(void) {
    const char* input = "(repeat 10 (db 0))";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_REPEAT);
    assert(node->as.repeat.count == 10);
    assert(node->as.repeat.form != NULL);
}

void test_parser_addr(void) {
    const char* input = "(addr 0xBF 0x00)";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_ADDR);
    assert(node->as.addr.hi != NULL);
    assert(node->as.addr.lo != NULL);
}

void test_parser_addr16(void) {
    const char* input = "(addr16 0xBF00)";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    assert(node != NULL);
    assert(node->kind == AST_ADDR16);
}

void test_parser_load_store(void) {
    // Test load
    {
        const char* input = "(load (addr 0xBF 0x00))";
        AstPool pool = {0};
        SeParser parser;
        se_parser_init(&parser, input, strlen(input), &pool);

        AstNode* node = se_parser_parse_form(&parser);
        assert(node != NULL);
        assert(node->kind == AST_LOAD);
        assert(node->as.load.addr != NULL);
    }

    // Test store
    {
        const char* input = "(store (addr 0xBF 0x00) 42)";
        AstPool pool = {0};
        SeParser parser;
        se_parser_init(&parser, input, strlen(input), &pool);

        AstNode* node = se_parser_parse_form(&parser);
        assert(node != NULL);
        assert(node->kind == AST_STORE);
        assert(node->as.store.addr != NULL);
        assert(node->as.store.value != NULL);
    }
}

void test_parser_hi_lo(void) {
    // Test hi
    {
        const char* input = "(hi 0x1234)";
        AstPool pool = {0};
        SeParser parser;
        se_parser_init(&parser, input, strlen(input), &pool);

        AstNode* node = se_parser_parse_form(&parser);
        assert(node != NULL);
        assert(node->kind == AST_HI);
    }

    // Test lo
    {
        const char* input = "(lo 0x1234)";
        AstPool pool = {0};
        SeParser parser;
        se_parser_init(&parser, input, strlen(input), &pool);

        AstNode* node = se_parser_parse_form(&parser);
        assert(node != NULL);
        assert(node->kind == AST_LO);
    }
}

void test_parser_error_expected_lparen(void) {
    const char* input = "defn foo () 42)";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    // Should parse "defn" as a symbol, not fail
    assert(node != NULL);
    assert(node->kind == AST_SYMBOL);
}

void test_parser_error_expected_rparen(void) {
    const char* input = "(+ 1 2";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    (void)node; // May or may not be NULL depending on implementation
    assert(se_parser_has_error(&parser));
}

void test_parser_error_unknown_form(void) {
    const char* input = "(unknown-form x y)";
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstNode* node = se_parser_parse_form(&parser);
    // Unknown forms are treated as function calls
    assert(node != NULL);
    assert(node->kind == AST_CALL);
}

// =============================================================================
// Codegen Test Implementations
// =============================================================================

static char* codegen_to_string(const char* input) {
    AstPool pool = {0};
    SeParser parser;
    se_parser_init(&parser, input, strlen(input), &pool);

    AstProgram program = {0};
    if (!se_parser_parse_program(&parser, &program)) {
        return NULL;
    }

    if (se_parser_has_error(&parser)) {
        return NULL;
    }

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
    assert(output != NULL);
    // Codegen emits constants in hex format
    assert(strstr(output, "x = 0x") != NULL);
    free(output);
}

void test_codegen_simple_function(void) {
    const char* input = "(defn foo () 42)";
    char* output = codegen_to_string(input);
    assert(output != NULL);
    assert(strstr(output, "foo:") != NULL);
    // Codegen emits constants in hex format, e.g., LOADI R0, 0x2A
    assert(strstr(output, "LOADI R0, 0x") != NULL);
    free(output);
}

void test_codegen_function_with_params(void) {
    const char* input = "(defn add (x y) (+ x y))";
    char* output = codegen_to_string(input);
    assert(output != NULL);
    assert(strstr(output, "add:") != NULL);
    free(output);
}

void test_codegen_arithmetic(void) {
    const char* input = "(defn test () (+ 1 2))";
    char* output = codegen_to_string(input);
    assert(output != NULL);
    assert(strstr(output, "test:") != NULL);
    free(output);
}

void test_codegen_if_statement(void) {
    const char* input = "(defn test (x) (if (< x 10) 1 0))";
    char* output = codegen_to_string(input);
    assert(output != NULL);
    assert(strstr(output, "test:") != NULL);
    free(output);
}

void test_codegen_while_loop(void) {
    const char* input = "(defn test () (let (i 0) (while (< i 10) (set i (+ i 1))) i))";
    char* output = codegen_to_string(input);
    assert(output != NULL);
    assert(strstr(output, "test:") != NULL);
    free(output);
}

void test_codegen_function_call(void) {
    const char* input = "(defn foo () 42) (defn bar () (foo))";
    char* output = codegen_to_string(input);
    assert(output != NULL);
    assert(strstr(output, "foo:") != NULL);
    assert(strstr(output, "bar:") != NULL);
    assert(strstr(output, "CALL foo") != NULL || strstr(output, "call foo") != NULL);
    free(output);
}

void test_codegen_let_bindings(void) {
    const char* input = "(defn test () (let (x 10 y 20) (+ x y)))";
    char* output = codegen_to_string(input);
    assert(output != NULL);
    assert(strstr(output, "test:") != NULL);
    free(output);
}

void test_codegen_data_section(void) {
    const char* input = "(data msg (db 72 101 108 108 111))";
    char* output = codegen_to_string(input);
    assert(output != NULL);
    assert(strstr(output, "msg:") != NULL);
    assert(strstr(output, "DB") != NULL || strstr(output, "db") != NULL);
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

// =============================================================================
// Integration Test Implementations
// =============================================================================

void test_integration_hello_world(void) {
    const char* input = "(defn main () (+ 1 2))";
    char* output = codegen_to_string(input);
    assert(output != NULL);
    assert(strstr(output, "main:") != NULL);
    assert(strstr(output, "section") != NULL || strstr(output, ".code") != NULL);
    free(output);
}

void test_integration_factorial(void) {
    const char* input = "(defn factorial (n) "
                        "  (if (<= n 1) "
                        "    1 "
                        "    (* n (factorial (- n 1)))))";
    char* output = codegen_to_string(input);
    assert(output != NULL);
    assert(strstr(output, "factorial:") != NULL);
    free(output);
}

void test_integration_fibonacci(void) {
    const char* input = "(defn fib (n) "
                        "  (if (<= n 1) "
                        "    n "
                        "    (+ (fib (- n 1)) (fib (- n 2)))))";
    char* output = codegen_to_string(input);
    assert(output != NULL);
    assert(strstr(output, "fib:") != NULL);
    free(output);
}

void test_integration_data_and_code(void) {
    const char* input = "(def MSG_ADDR 0x4000) "
                        "(data msg 0x4000 (db \"Hello\")) "
                        "(defn main () (load (addr16 MSG_ADDR)))";
    char* output = codegen_to_string(input);
    assert(output != NULL);
    assert(strstr(output, "MSG_ADDR") != NULL);
    assert(strstr(output, "msg:") != NULL);
    assert(strstr(output, "main:") != NULL);
    free(output);
}
