#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "memory.h"

#define TINY16_PARSER_MAX_LABEL_NAME_LENGTH 256
#define TINY16_PARSER_MAX_LABELS            4096
#define TINY16_PARSER_LINE_BUFFER_SIZE      256

typedef struct {
    char name[TINY16_PARSER_MAX_LABEL_NAME_LENGTH];
    uint16_t addr;
} Tiny16Parser;

typedef enum {
    TINY16_PARSER_SECTION_UNKNOWN = -1,
    TINY16_PARSER_SECTION_CODE,
    TINY16_PARSER_SECTION_DATA
} tiny16_parser_section_t;

typedef struct {
    Tiny16Parser labels[TINY16_PARSER_MAX_LABELS];
    int label_count;

    FILE* output_file;
    size_t output_file_size;

    FILE* source_file;
    int source_line_no;
    char* source_line;

    tiny16_parser_section_t current_section;

    uint16_t code_pc;
    uint16_t data_pc;

    uint8_t data_buffer[TINY16_DATA_END - TINY16_DATA_BEGIN + 1];
    uint16_t data_size;
} Tiny16AsmContext;

#define TINY16_PARSER_LABEL_NOT_FOUND 0xFFFF

void tiny16_parser_strip_comment(Tiny16AsmContext* ctx);
void tiny16_parser_trim_left(Tiny16AsmContext* ctx);
void tiny16_parser_trim_right(Tiny16AsmContext* ctx);
int tiny16_parser_label_length(Tiny16AsmContext* ctx);
tiny16_parser_section_t tiny16_parser_section(Tiny16AsmContext* ctx);

uint16_t tiny16_parser_label_addr(Tiny16AsmContext* ctx, char* name);

Tiny16OpCode tiny16_parser_parse_mnemonic(Tiny16AsmContext* ctx);
uint8_t tiny16_parser_parse_reg(Tiny16AsmContext* ctx);
uint16_t tiny16_parser_parse_imm(Tiny16AsmContext* ctx);
uint8_t tiny16_parser_parse_imm8(Tiny16AsmContext* ctx);
void tiny16_parser_skip_sep(Tiny16AsmContext* ctx);
size_t tiny16_parser_read_token(Tiny16AsmContext* ctx, char* buf, size_t max_len);
void tiny16_parser_expect_end(Tiny16AsmContext* ctx);
long tiny16_parser_parse_number(Tiny16AsmContext* ctx);
Tiny16Addr tiny16_parser_parse_addr(Tiny16AsmContext* ctx);
void tiny16_parser_parse_db(Tiny16AsmContext* ctx);
int tiny16_parser_parse_times_prefix(Tiny16AsmContext* ctx);
void tiny16_parser_eat_space(Tiny16AsmContext* ctx);
void tiny16_parser_eat_char(Tiny16AsmContext* ctx, char c);

bool tiny16_parser_preprocess_line(Tiny16AsmContext* ctx, char* buffer);
bool tiny16_parser_parse_section(Tiny16AsmContext* ctx);
bool tiny16_parser_skip_label(Tiny16AsmContext* ctx);
void tiny16_parser_parse_data(Tiny16AsmContext* ctx);

typedef void (*tiny16_parser_callback_fn)(Tiny16AsmContext* ctx);
void tiny16_parser_times_do(Tiny16AsmContext* ctx, int times, tiny16_parser_callback_fn callback);

void tiny16_parser_emit_code(Tiny16AsmContext* ctx);
void tiny16_parser_emit_data(Tiny16AsmContext* ctx);
