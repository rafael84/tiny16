#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "memory.h"

#define TINY16_ASM_MAX_LABEL_NAME_LENGTH 256
#define TINY16_ASM_MAX_LABELS 4096

typedef struct {
    char name[TINY16_ASM_MAX_LABEL_NAME_LENGTH];
    uint16_t addr;
} Tiny16Asm;

typedef enum section { SECTION_UNKNOWN = -1, SECTION_CODE, SECTION_DATA } tiny16_asm_section_t;

typedef struct {

    Tiny16Asm labels[TINY16_ASM_MAX_LABELS];
    int label_count;

    FILE* output_file;
    size_t output_file_size;

    FILE* source_file;
    int source_line_no;
    char* source_line;

    tiny16_asm_section_t current_section;

    uint16_t code_pc;
    uint16_t data_pc;

    uint8_t data_buffer[TINY16_MEMORY_DATA_END - TINY16_MEMORY_DATA_BEGIN];
    uint16_t data_size;
} Tiny16AsmContext;

#define TINY16_ASM_LABEL_NOT_FOUND 0xFFFF

uint16_t tiny16_asm_ctx_label_addr(Tiny16AsmContext* ctx, char* name);

Tiny16OpCode tiny16_asm_ctx_parse_mnemonic(Tiny16AsmContext* ctx, char* str, char** saveptr);
uint8_t tiny16_asm_ctx_parse_reg(Tiny16AsmContext* ctx, char* str, char** saveptr);
uint16_t tiny16_asm_ctx_parse_imm(Tiny16AsmContext* ctx, char* str, char** saveptr);
uint8_t tiny16_asm_ctx_parse_imm8(Tiny16AsmContext* ctx, char* str, char** saveptr);
void tiny16_asm_ctx_parse_db(Tiny16AsmContext* ctx);

void tiny16_asm_ctx_emit_code(Tiny16AsmContext* ctx);
void tiny16_asm_ctx_emit_data(Tiny16AsmContext* ctx);
