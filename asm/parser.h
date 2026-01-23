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
#define TINY16_PARSER_MAX_LABELS       4096

typedef struct {
    char name[TINY16_PARSER_MAX_TOKEN_LENGTH];
    uint16_t addr;
} Tiny16Label;

typedef enum {
    TINY16_PARSER_SECTION_UNKNOWN = -1,
    TINY16_PARSER_SECTION_CODE,
    TINY16_PARSER_SECTION_DATA
} tiny16_parser_section_t;

typedef struct {
    const char* source_filename;

    Tiny16Label labels[TINY16_PARSER_MAX_LABELS];
    int label_count;

    FILE* output_file;
    size_t output_file_size;

    Lexer lexer;
    Token current_token;

    tiny16_parser_section_t current_section;

    uint16_t code_pc;
    uint16_t data_pc;

    uint8_t data_buffer[TINY16_DATA_END - TINY16_DATA_BEGIN + 1];
    uint16_t data_size;
} Tiny16Parser;

#define TINY16_PARSER_LABEL_NOT_FOUND UINT16_MAX

void tiny16_parser_next(Tiny16Parser* parser);
void tiny16_parser_skip_trivia(Tiny16Parser* parser);
void tiny16_parser_expect(Tiny16Parser* parser, TokenKind kind);
bool tiny16_parser_match(Tiny16Parser* parser, TokenKind kind);
void tiny16_parser_skip_to_eol(Tiny16Parser* parser);

bool tiny16_parser_parse_label(Tiny16Parser* parser);
bool tiny16_parser_skip_label(Tiny16Parser* parser);
uint16_t tiny16_parser_label_addr(Tiny16Parser* parser, const char* name, size_t len);

bool tiny16_parser_parse_section(Tiny16Parser* parser);
uint16_t tiny16_parser_parse_times_prefix(Tiny16Parser* parser);

Tiny16OpCode tiny16_parser_parse_mnemonic(Tiny16Parser* parser);
uint8_t tiny16_parser_parse_reg(Tiny16Parser* parser);
Tiny16AddrPair tiny16_parser_parse_reg_pair(Tiny16Parser* parser);
uint16_t tiny16_parser_parse_imm(Tiny16Parser* parser);
uint8_t tiny16_parser_parse_imm8(Tiny16Parser* parser);
Tiny16Addr tiny16_parser_parse_addr(Tiny16Parser* parser);

void tiny16_parser_parse_data(Tiny16Parser* parser);
void tiny16_parser_emit_code(Tiny16Parser* parser);
void tiny16_parser_emit_data(Tiny16Parser* parser);
