#pragma once

#include "../vm/memory.h"
#include "ast.h"

#include <stdbool.h>
#include <stdio.h>

#define SE_MAX_LOCALS      32
#define SE_MAX_LABEL_ID    9999
#define SE_MAX_DATA_LABELS 128

typedef struct {
    char name[SE_MAX_SYMBOL_LEN];
    int32_t value;
    bool is_function;
} SeConstant;

typedef struct {
    char name[SE_MAX_SYMBOL_LEN];
    int offset; // Stack offset from FP (params) or R2:R3 base (let locals)
} SeLocal;

typedef struct {
    char name[SE_MAX_SYMBOL_LEN];
    int32_t addr; // Computed address
    int32_t size; // Size in bytes
} SeDataLabel;

typedef struct {
    FILE* output;
    const char* filename;

    SeConstant constants[SE_MAX_CONSTANTS];
    size_t constant_count;

    char functions[SE_MAX_FUNCTIONS][SE_MAX_SYMBOL_LEN];
    size_t function_count;

    SeDataLabel data_labels[SE_MAX_DATA_LABELS];
    size_t data_label_count;
    int32_t data_base_addr;    // Base address for data section
    int32_t data_current_addr; // Current address during data collection

    SeLocal locals[SE_MAX_LOCALS];
    size_t local_count;
    size_t param_count;

    int let_depth;       // Nesting level of let expressions (0 = not in let)
    int let_stack_depth; // Current stack depth from R2:R3 base (for negative offsets)

    int label_counter;

    bool has_error;
    char error_msg[256];
    size_t error_line;
} SeCodegen;

void se_codegen_init(SeCodegen* cg, FILE* output, const char* filename);

// First pass: collect constants and function names
bool se_codegen_collect(SeCodegen* cg, AstProgram* program);

// Second pass: emit assembly
bool se_codegen_emit(SeCodegen* cg, AstProgram* program);

bool se_codegen_has_error(SeCodegen* cg);
void se_codegen_print_error(SeCodegen* cg);
