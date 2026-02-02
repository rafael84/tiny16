#pragma once

#include "ast.h"
#include "lexer.h"

typedef enum {
    SE_PARSE_OK = 0,
    SE_PARSE_ERROR_UNEXPECTED_TOKEN,
    SE_PARSE_ERROR_EXPECTED_LPAREN,
    SE_PARSE_ERROR_EXPECTED_RPAREN,
    SE_PARSE_ERROR_EXPECTED_SYMBOL,
    SE_PARSE_ERROR_EXPECTED_NUMBER,
    SE_PARSE_ERROR_UNKNOWN_FORM,
    SE_PARSE_ERROR_TOO_MANY_ARGS,
    SE_PARSE_ERROR_TOO_FEW_ARGS,
    SE_PARSE_ERROR_OUT_OF_MEMORY,
} SeParseError;

const char* se_parse_error_name(SeParseError error);

typedef struct {
    SeLexer lexer;
    SeToken current;
    AstPool* pool;
    SeParseError error;
    size_t error_line;
    size_t error_column;
    char error_msg[256];
} SeParser;

void se_parser_init(SeParser* parser, const char* source, size_t source_len, AstPool* pool);

bool se_parser_parse_program(SeParser* parser, AstProgram* program);
bool se_parser_parse_ns_with_requires(SeParser* parser, AstProgram* program);
AstNode* se_parser_parse_form(SeParser* parser);

bool se_parser_has_error(SeParser* parser);
void se_parser_print_error(SeParser* parser, const char* filename);
