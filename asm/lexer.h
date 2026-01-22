#pragma once

#include <stddef.h>

typedef enum {
    TOKEN_END = 0,
    TOKEN_INVALID,
    TOKEN_COMMENT,
    TOKEN_SYMBOL,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_KEYWORD,
    TOKEN_NUMBER,
    TOKEN_OPEN_BRACKET,
    TOKEN_CLOSE_BRACKET,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_DOT,
} TokenKind;

const char* token_kind_name(TokenKind kind);

typedef struct {
    TokenKind kind;
    const char* text;
    size_t text_len;
} Token;

typedef struct {
    const char* content;
    size_t content_len;
    size_t cursor;
    size_t line_start;
    size_t line;
} Lexer;

Lexer lexer_new(const char* content, size_t content_len);
Token lexer_next(Lexer* lexer);
