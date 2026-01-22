#include "lexer.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    TokenKind kind;
    const char* text;
} LiteralToken;

LiteralToken literal_tokens[] = {
    {.text = ",", .kind = TOKEN_COMMA},        {.text = ":", .kind = TOKEN_COLON},
    {.text = "[", .kind = TOKEN_OPEN_BRACKET}, {.text = "]", .kind = TOKEN_CLOSE_BRACKET},
    {.text = "+", .kind = TOKEN_PLUS},         {.text = "-", .kind = TOKEN_MINUS},
    {.text = ".", .kind = TOKEN_DOT},
};

#define literal_tokens_count (sizeof literal_tokens / sizeof literal_tokens[0])

const char* keywords[] = {"section", "times"};

#define keywords_count (sizeof keywords / sizeof keywords[0])

const char* token_kind_name(TokenKind kind) {
    switch (kind) {
    case TOKEN_END:
        return "end of content";
    case TOKEN_COMMENT:
        return "comment";
    case TOKEN_SYMBOL:
        return "symbol";
    case TOKEN_INVALID:
        return "invalid token";
    case TOKEN_COMMA:
        return "comma";
    case TOKEN_COLON:
        return "colon";
    case TOKEN_KEYWORD:
        return "keyword";
    case TOKEN_NUMBER:
        return "number";
    case TOKEN_OPEN_BRACKET:
        return "open bracket";
    case TOKEN_CLOSE_BRACKET:
        return "close bracket";
    case TOKEN_PLUS:
        return "plus";
    case TOKEN_MINUS:
        return "minus";
    case TOKEN_DOT: // REVIEW this
        return "dot";
    default:
        assert(0 && "unreachable");
    }
}

Lexer lexer_new(const char* content, size_t content_len) {
    Lexer lexer = {0};
    lexer.content = content;
    lexer.content_len = content_len;
    return lexer;
}

bool lexer_starts_with(Lexer* lexer, const char* prefix) {
    size_t prefix_len = strlen(prefix);
    if (prefix_len == 0) return true;
    if (lexer->cursor + prefix_len - 1 >= lexer->content_len) return false;
    for (size_t i = 0; i < prefix_len; ++i) {
        if (prefix[i] != lexer->content[lexer->cursor + i]) return false;
    }
    return true;
}

void lexer_eat_chars(Lexer* lexer, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        assert(lexer->cursor < lexer->content_len);
        char c = lexer->content[lexer->cursor];
        lexer->cursor += 1;
        if (c == '\n') {
            lexer->line += 1;
            lexer->line_start = lexer->cursor;
        }
    }
}

void lexer_trim_left(Lexer* lexer) {
    while (lexer->cursor < lexer->content_len && isspace(lexer->content[lexer->cursor])) {
        lexer_eat_chars(lexer, 1);
    }
}

bool is_symbol_start(char c) { return isalpha(c) || c == '_'; }
bool is_symbol(char c) { return isalnum(c) || c == '_'; }

Token lexer_next(Lexer* lexer) {
    lexer_trim_left(lexer);

    Token token = {.text = &lexer->content[lexer->cursor]};

    if (lexer->cursor >= lexer->content_len) return token;

    if (lexer->content[lexer->cursor] == ';') {
        token.kind = TOKEN_COMMENT;
        while (lexer->cursor < lexer->content_len && lexer->content[lexer->cursor] != '\n')
            lexer_eat_chars(lexer, 1);
        if (lexer->cursor < lexer->content_len) lexer_eat_chars(lexer, 1);
        token.text_len = &lexer->content[lexer->cursor] - token.text;
        if (token.text[token.text_len - 1] == '\n') token.text_len -= 1; // REVIEW this
        return token;
    }

    if (isdigit(lexer->content[lexer->cursor])) {
        token.kind = TOKEN_NUMBER;
        if (lexer->content[lexer->cursor] == '0') {
            if (lexer_starts_with(lexer, "0x") || lexer_starts_with(lexer, "0X")) {
                lexer_eat_chars(lexer, 2);
                token.text_len += 2;
                size_t digits = 0;
                while (lexer->cursor < lexer->content_len &&
                       isxdigit(lexer->content[lexer->cursor])) {
                    lexer_eat_chars(lexer, 1);
                    token.text_len += 1;
                    digits += 1;
                }
                if (!digits) {
                    token.kind = TOKEN_INVALID;
                    token.text_len = 2;
                }
                return token;
            } else if (lexer_starts_with(lexer, "0b") || lexer_starts_with(lexer, "0B")) {
                lexer_eat_chars(lexer, 2);
                token.text_len += 2;
                size_t digits = 0;
                while (lexer->cursor < lexer->content_len &&
                       (lexer->content[lexer->cursor] == '0' ||
                        lexer->content[lexer->cursor] == '1')) {
                    lexer_eat_chars(lexer, 1);
                    token.text_len += 1;
                    digits += 1;
                }
                if (!digits) {
                    token.kind = TOKEN_INVALID;
                    token.text_len = 2;
                }
                return token;
            }
        }
        while (lexer->cursor < lexer->content_len && isdigit(lexer->content[lexer->cursor])) {
            lexer_eat_chars(lexer, 1);
            token.text_len += 1;
        }
        return token;
    }

    for (size_t i = 0; i < literal_tokens_count; ++i) {
        if (lexer_starts_with(lexer, literal_tokens[i].text)) {
            size_t text_len = strlen(literal_tokens[i].text);
            token.kind = literal_tokens[i].kind;
            token.text_len = text_len;
            lexer_eat_chars(lexer, text_len);
            return token;
        }
    }

    if (is_symbol_start(lexer->content[lexer->cursor])) {
        token.kind = TOKEN_SYMBOL;
        while (lexer->cursor < lexer->content_len && is_symbol(lexer->content[lexer->cursor])) {
            lexer_eat_chars(lexer, 1);
            token.text_len += 1;
        }
        for (size_t i = 0; i < keywords_count; ++i) {
            size_t keyword_len = strlen(keywords[i]);
            if (keyword_len == token.text_len &&
                memcmp(keywords[i], token.text, keyword_len) == 0) {
                token.kind = TOKEN_KEYWORD;
                break;
            }
        }
        return token;
    }

    lexer_eat_chars(lexer, 1);
    token.kind = TOKEN_INVALID;
    token.text_len = 1;
    return token;
}
