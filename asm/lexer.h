#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    TOKEN_END = 0,
    TOKEN_EOL,
    TOKEN_INVALID,
    TOKEN_COMMENT,
    TOKEN_SYMBOL,
    TOKEN_STRING,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_KEYWORD,
    TOKEN_NUMBER,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_DOT,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_EQUALS,
    TOKEN_TIMES,
    TOKEN_DIV,
    TOKEN_MOD,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_XOR,
    TOKEN_NOT,
    TOKEN_SHL,
    TOKEN_SHR,
} TokenKind;

const char* token_kind_name(TokenKind kind);

typedef struct {
    TokenKind kind;
    const char* text;
    size_t text_len;
    size_t line;
    size_t column;
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

bool str_eq_ci(const char* a, const char* b, size_t n);
