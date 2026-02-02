#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "lexer.h"
#include "memory.h"

#define TINY16_PARSER_MAX_TOKEN_LENGTH 256
#define TINY16_PARSER_MAX_SYMBOLS      8192
#define TINY16_PARSER_MAX_ERROR_MSG    512
#define TINY16_PARSER_MAX_FILENAME     1024

typedef enum {
    TINY16_PARSER_OK = 0,
    TINY16_PARSER_ERROR_UNEXPECTED_TOKEN,
    TINY16_PARSER_ERROR_LABEL_TOO_LONG,
    TINY16_PARSER_ERROR_DUPLICATE_LABEL,
    TINY16_PARSER_ERROR_TOO_MANY_LABELS,
    TINY16_PARSER_ERROR_CONST_TOO_LONG,
    TINY16_PARSER_ERROR_DUPLICATE_CONST,
    TINY16_PARSER_ERROR_TOO_MANY_CONSTS,
    TINY16_PARSER_ERROR_SYMBOL_ALREADY_DEFINED,
    TINY16_PARSER_ERROR_UNKNOWN_SECTION,
    TINY16_PARSER_ERROR_INVALID_NUMBER,
    TINY16_PARSER_ERROR_OUT_OF_RANGE,
    TINY16_PARSER_ERROR_UNKNOWN_MNEMONIC,
    TINY16_PARSER_ERROR_INVALID_REGISTER,
    TINY16_PARSER_ERROR_EXPECTED_EVEN_REGISTER,
    TINY16_PARSER_ERROR_WRONG_REGISTER,
    TINY16_PARSER_ERROR_UNDEFINED_SYMBOL,
    TINY16_PARSER_ERROR_UNKNOWN_DIRECTIVE,
    TINY16_PARSER_ERROR_PROGRAM_TOO_LARGE,
    TINY16_PARSER_ERROR_ORG_REWIND_NOT_ALLOWED,
} Tiny16ParserError;

typedef enum {
    TINY16_SYMBOL_LABEL = 0,
    TINY16_SYMBOL_CONST = 1,
} Tiny16SymbolKind;

typedef struct {
    char name[TINY16_PARSER_MAX_TOKEN_LENGTH];
    Tiny16SymbolKind kind;
    uint16_t value;
} Tiny16Symbol;

typedef enum {
    TINY16_PARSER_SECTION_UNKNOWN = -1,
    TINY16_PARSER_SECTION_CODE,
    TINY16_PARSER_SECTION_DATA
} tiny16_parser_section_t;

typedef struct {
    const char* source_filename;
    char current_filename[TINY16_PARSER_MAX_FILENAME];
    int64_t line_base;

    Tiny16Symbol* symbols;
    int symbol_count;
    int symbol_capacity;

    FILE* output_file;
    size_t output_file_size;

    Lexer lexer;
    Token current_token;

    tiny16_parser_section_t current_section;

    uint16_t code_pc;
    uint16_t data_pc;

    uint8_t data_buffer[TINY16_DATA_END - TINY16_DATA_BEGIN + 1];
    uint16_t data_size;

    Tiny16ParserError error;
    size_t error_line;
    char error_msg[TINY16_PARSER_MAX_ERROR_MSG];
} Tiny16Parser;

#define TINY16_PARSER_LABEL_NOT_FOUND UINT16_MAX

void tiny16_parser_init(Tiny16Parser* parser);
void tiny16_parser_free(Tiny16Parser* parser);

bool tiny16_parser_has_error(const Tiny16Parser* parser);
void tiny16_parser_print_error(const Tiny16Parser* parser);

void tiny16_parser_next(Tiny16Parser* parser);
void tiny16_parser_skip_trivia(Tiny16Parser* parser);
void tiny16_parser_expect(Tiny16Parser* parser, TokenKind kind);
bool tiny16_parser_match(Tiny16Parser* parser, TokenKind kind);
void tiny16_parser_skip_to_eol(Tiny16Parser* parser);

bool tiny16_parser_parse_label(Tiny16Parser* parser);
bool tiny16_parser_skip_label(Tiny16Parser* parser);
uint16_t tiny16_parser_label_addr(Tiny16Parser* parser, const char* name, size_t len);

bool tiny16_parser_parse_const(Tiny16Parser* parser);

bool tiny16_parser_parse_section(Tiny16Parser* parser);
bool tiny16_parser_parse_org(Tiny16Parser* parser);
uint16_t tiny16_parser_parse_times_prefix(Tiny16Parser* parser);
bool tiny16_parser_parse_line_directive(Tiny16Parser* parser);
bool tiny16_parser_parse_file_directive(Tiny16Parser* parser);

Tiny16OpCode tiny16_parser_parse_mnemonic(Tiny16Parser* parser);
uint8_t tiny16_parser_parse_reg(Tiny16Parser* parser);
Tiny16AddrPair tiny16_parser_parse_reg_pair(Tiny16Parser* parser);
uint16_t tiny16_parser_parse_imm(Tiny16Parser* parser);
uint8_t tiny16_parser_parse_imm8(Tiny16Parser* parser);
Tiny16Addr tiny16_parser_parse_addr(Tiny16Parser* parser);

void tiny16_parser_parse_data(Tiny16Parser* parser);
void tiny16_parser_emit_code(Tiny16Parser* parser);
void tiny16_parser_emit_data(Tiny16Parser* parser);

long tiny16_parser_parse_expression(Tiny16Parser* parser);
