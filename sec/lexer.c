#include "lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

const char* se_token_kind_name(SeTokenKind kind) {
    switch (kind) {
    case SE_TOKEN_END: return "END";
    case SE_TOKEN_LPAREN: return "LPAREN";
    case SE_TOKEN_RPAREN: return "RPAREN";
    case SE_TOKEN_NUMBER: return "NUMBER";
    case SE_TOKEN_STRING: return "STRING";
    case SE_TOKEN_SYMBOL: return "SYMBOL";
    case SE_TOKEN_KEYWORD: return "KEYWORD";
    case SE_TOKEN_NIL: return "NIL";
    case SE_TOKEN_TRUE: return "TRUE";
    case SE_TOKEN_FALSE: return "FALSE";
    case SE_TOKEN_INVALID: return "INVALID";
    default: return "UNKNOWN";
    }
}

SeLexer se_lexer_new(const char* content, size_t content_len) {
    SeLexer lexer = {
        .content = content,
        .content_len = content_len,
        .cursor = 0,
        .line_start = 0,
        .line = 1,
    };
    return lexer;
}

static char peek_char(SeLexer* lexer) {
    if (lexer->cursor >= lexer->content_len) return '\0';
    return lexer->content[lexer->cursor];
}

static char next_char(SeLexer* lexer) {
    if (lexer->cursor >= lexer->content_len) return '\0';
    char c = lexer->content[lexer->cursor++];
    if (c == '\n') {
        lexer->line++;
        lexer->line_start = lexer->cursor;
    }
    return c;
}

static void skip_whitespace_and_comments(SeLexer* lexer) {
    while (lexer->cursor < lexer->content_len) {
        char c = peek_char(lexer);
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            next_char(lexer);
        } else if (c == ';') {
            // Comment: skip to end of line
            while (lexer->cursor < lexer->content_len && peek_char(lexer) != '\n') {
                next_char(lexer);
            }
        } else {
            break;
        }
    }
}

static bool is_symbol_start(char c) {
    if (isalpha((unsigned char)c)) return true;
    if (c == '_' || c == '-' || c == '+' || c == '*' || c == '/' || c == '!' || c == '?' ||
        c == '<' || c == '>' || c == '=' || c == '&' || c == '|' || c == '^' || c == '~' ||
        c == '@' || c == '#' || c == '$' || c == '%' || c == '{')
        return true;
    return false;
}

static bool is_keyword_char(char c) {
    if (isalnum((unsigned char)c)) return true;
    if (c == '_' || c == '-' || c == '?') return true;
    return false;
}

static bool is_symbol_char(char c) {
    if (is_symbol_start(c)) return true;
    if (isdigit((unsigned char)c)) return true;
    if (c == '.' || c == '{' || c == '}') return true;
    return false;
}

static SeToken make_token(SeLexer* lexer, SeTokenKind kind, size_t start, size_t len) {
    SeToken token = {
        .kind = kind,
        .text = lexer->content + start,
        .text_len = len,
        .line = lexer->line,
        .column = start - lexer->line_start + 1,
        .number_value = 0,
    };
    return token;
}

static SeToken scan_number(SeLexer* lexer) {
    size_t start = lexer->cursor;
    size_t start_line = lexer->line;
    size_t start_col = lexer->cursor - lexer->line_start + 1;

    bool is_hex = false;
    bool is_negative = false;
    bool is_fixed_point = false;

    if (peek_char(lexer) == '-') {
        is_negative = true;
        next_char(lexer);
    }

    if (peek_char(lexer) == '0' && lexer->cursor + 1 < lexer->content_len) {
        char next = lexer->content[lexer->cursor + 1];
        if (next == 'x' || next == 'X') {
            is_hex = true;
            next_char(lexer); // skip '0'
            next_char(lexer); // skip 'x'
        }
    }

    while (lexer->cursor < lexer->content_len) {
        char c = peek_char(lexer);
        if (is_hex) {
            if (!isxdigit((unsigned char)c)) break;
        } else {
            if (!isdigit((unsigned char)c)) break;
        }
        next_char(lexer);
    }

    // Check for fixed-point: N.N (only for decimal, not hex)
    if (!is_hex && lexer->cursor < lexer->content_len && peek_char(lexer) == '.') {
        // Look ahead to make sure there's a digit after the dot
        if (lexer->cursor + 1 < lexer->content_len &&
            isdigit((unsigned char)lexer->content[lexer->cursor + 1])) {
            is_fixed_point = true;
            next_char(lexer); // skip '.'
            while (lexer->cursor < lexer->content_len && isdigit((unsigned char)peek_char(lexer))) {
                next_char(lexer);
            }
        }
    }

    size_t len = lexer->cursor - start;
    SeToken token = {
        .kind = SE_TOKEN_NUMBER,
        .text = lexer->content + start,
        .text_len = len,
        .line = start_line,
        .column = start_col,
    };

    const char* num_start = lexer->content + start;
    if (is_negative) num_start++;

    if (is_fixed_point) {
        // Parse as 8.8 fixed-point: integer part * 256 + fraction * 256
        // Use strtod to parse the full decimal number, then convert
        char buf[64];
        size_t buf_len = (size_t)(lexer->content + lexer->cursor - num_start);
        if (buf_len >= sizeof(buf)) buf_len = sizeof(buf) - 1;
        memcpy(buf, num_start, buf_len);
        buf[buf_len] = '\0';
        double val = strtod(buf, NULL);
        // Convert to 8.8 fixed-point: multiply by 256 and round
        int32_t fixed = (int32_t)(val * 256.0 + 0.5);
        if (is_negative) fixed = -fixed;
        token.number_value = fixed;
    } else if (is_hex) {
        token.number_value = (int32_t)strtol(num_start, NULL, 16);
        if (is_negative) token.number_value = -token.number_value;
    } else {
        token.number_value = (int32_t)strtol(num_start, NULL, 10);
        if (is_negative) token.number_value = -token.number_value;
    }

    return token;
}

static SeToken scan_string(SeLexer* lexer) {
    size_t start = lexer->cursor;
    size_t start_line = lexer->line;
    size_t start_col = lexer->cursor - lexer->line_start + 1;

    next_char(lexer); // skip opening quote

    while (lexer->cursor < lexer->content_len) {
        char c = peek_char(lexer);
        if (c == '"') {
            next_char(lexer); // skip closing quote
            break;
        }
        if (c == '\\' && lexer->cursor + 1 < lexer->content_len) {
            next_char(lexer); // skip backslash
        }
        next_char(lexer);
    }

    return (SeToken){
        .kind = SE_TOKEN_STRING,
        .text = lexer->content + start,
        .text_len = lexer->cursor - start,
        .line = start_line,
        .column = start_col,
    };
}

static SeToken scan_keyword(SeLexer* lexer) {
    size_t start = lexer->cursor;
    size_t start_line = lexer->line;
    size_t start_col = lexer->cursor - lexer->line_start + 1;

    next_char(lexer); /* consume ':' */

    while (lexer->cursor < lexer->content_len && is_keyword_char(peek_char(lexer))) {
        next_char(lexer);
    }

    return (SeToken){
        .kind = SE_TOKEN_KEYWORD,
        .text = lexer->content + start,
        .text_len = lexer->cursor - start,
        .line = start_line,
        .column = start_col,
    };
}

static SeToken scan_symbol(SeLexer* lexer) {
    size_t start = lexer->cursor;
    size_t start_line = lexer->line;
    size_t start_col = lexer->cursor - lexer->line_start + 1;

    while (lexer->cursor < lexer->content_len && is_symbol_char(peek_char(lexer))) {
        next_char(lexer);
    }

    size_t len = lexer->cursor - start;
    SeToken token = (SeToken){
        .kind = SE_TOKEN_SYMBOL,
        .text = lexer->content + start,
        .text_len = len,
        .line = start_line,
        .column = start_col,
    };

    /* Special literals: nil, true, false */
    if (len == 3 && strncmp(lexer->content + start, "nil", 3) == 0) token.kind = SE_TOKEN_NIL;
    if (len == 4 && strncmp(lexer->content + start, "true", 4) == 0) token.kind = SE_TOKEN_TRUE;
    if (len == 5 && strncmp(lexer->content + start, "false", 5) == 0) token.kind = SE_TOKEN_FALSE;

    return token;
}

SeToken se_lexer_next(SeLexer* lexer) {
    skip_whitespace_and_comments(lexer);

    if (lexer->cursor >= lexer->content_len) {
        return make_token(lexer, SE_TOKEN_END, lexer->cursor, 0);
    }

    char c = peek_char(lexer);
    size_t start = lexer->cursor;

    switch (c) {
    case '(': next_char(lexer); return make_token(lexer, SE_TOKEN_LPAREN, start, 1);
    case ')': next_char(lexer); return make_token(lexer, SE_TOKEN_RPAREN, start, 1);
    case '"': return scan_string(lexer);
    case ':': return scan_keyword(lexer);
    default:
        // Number or symbol
        if (isdigit((unsigned char)c)) {
            return scan_number(lexer);
        }
        // Negative number
        if (c == '-' && lexer->cursor + 1 < lexer->content_len &&
            isdigit((unsigned char)lexer->content[lexer->cursor + 1])) {
            return scan_number(lexer);
        }
        // Hex number starting with 0x
        if (c == '0' && lexer->cursor + 1 < lexer->content_len) {
            char next = lexer->content[lexer->cursor + 1];
            if (next == 'x' || next == 'X') {
                return scan_number(lexer);
            }
        }
        if (is_symbol_start(c)) return scan_symbol(lexer);

        // Invalid character
        next_char(lexer);
        return make_token(lexer, SE_TOKEN_INVALID, start, 1);
    }
}

SeToken se_lexer_peek(SeLexer* lexer) {
    SeLexer saved = *lexer;
    SeToken token = se_lexer_next(lexer);
    *lexer = saved;
    return token;
}

bool se_token_is_symbol(SeToken* token, const char* name) {
    if (token->kind != SE_TOKEN_SYMBOL) return false;
    size_t name_len = strlen(name);
    if (token->text_len != name_len) return false;
    return strncmp(token->text, name, name_len) == 0;
}
