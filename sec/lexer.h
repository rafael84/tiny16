#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// S-expression token types
typedef enum {
    SE_TOKEN_END = 0, // End of input
    SE_TOKEN_LPAREN,  // (
    SE_TOKEN_RPAREN,  // )
    SE_TOKEN_NUMBER,  // decimal or hex literal
    SE_TOKEN_STRING,  // "..."
    SE_TOKEN_SYMBOL,  // identifier or keyword
    SE_TOKEN_INVALID, // lexer error
} SeTokenKind;

const char* se_token_kind_name(SeTokenKind kind);

typedef struct {
    SeTokenKind kind;
    const char* text;
    size_t text_len;
    size_t line;
    size_t column;
    int32_t number_value;
} SeToken;

typedef struct {
    const char* content;
    size_t content_len;
    size_t cursor;
    size_t line_start;
    size_t line;
} SeLexer;

SeLexer se_lexer_new(const char* content, size_t content_len);
SeToken se_lexer_next(SeLexer* lexer);
SeToken se_lexer_peek(SeLexer* lexer);

bool se_token_is_symbol(SeToken* token, const char* name);
