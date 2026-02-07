#pragma once

#include "../vm/memory.h"
#include "ast.h"

#include <stdbool.h>
#include <stdio.h>

// Codegen limits (these are fixed-size for simplicity)
#define SE_MAX_LOCALS        32
#define SE_MAX_LABEL_ID      9999
#define SE_MAX_DATA_LABELS   128
#define SE_MAX_CONSTANTS     4096
#define SE_MAX_FUNCTIONS     1024
#define SE_MAX_KEYWORDS      256
#define SE_MAX_RECORDS       32
#define SE_MAX_RECORD_FIELDS 32

typedef struct {
    char name[SE_MAX_SYMBOL_LEN];
    int32_t value;
    bool is_function;
} SeConstant;

typedef struct {
    char name[SE_MAX_SYMBOL_LEN];
    int offset;     // Stack offset from FP (params) or R2:R3 base (let locals)
    bool is_16bit;  // True for u16/i16 values (occupies 2 stack slots)
    bool is_signed; // True for i8/i16 values
} SeLocal;

typedef struct {
    char name[SE_MAX_SYMBOL_LEN];
    int32_t addr;                        // Computed address
    int32_t size;                        // Size in bytes
    char record_type[SE_MAX_SYMBOL_LEN]; // Record type name (empty if not a record)
    int32_t element_count;               // Array element count (0 = not an array)
    int32_t element_size;                // Size of each array element in bytes
    bool is_16bit;                       // True for u16/i16 variables (2 bytes, no record/array)
    bool is_signed;                      // True for i8/i16 variables
} SeDataLabel;

typedef struct {
    char name[SE_MAX_SYMBOL_LEN];
    int32_t offset; // Byte offset from record base
    bool is_16bit;  // true for i16/u16 fields (2 bytes)
    bool is_signed; // true for i8/i16 fields
} SeRecordField;

typedef struct {
    char name[SE_MAX_SYMBOL_LEN];
    SeRecordField fields[SE_MAX_RECORD_FIELDS];
    size_t field_count;
    int32_t total_size; // Total size in bytes
} SeRecordType;

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

    char keywords[SE_MAX_KEYWORDS][SE_MAX_SYMBOL_LEN];
    size_t keyword_count;

    SeRecordType records[SE_MAX_RECORDS];
    size_t record_count;

    SeLocal locals[SE_MAX_LOCALS];
    size_t local_count;
    size_t param_count;

    int let_depth;       // Nesting level of let expressions (0 = not in let)
    int let_stack_depth; // Current stack depth from R2:R3 base (for negative offsets)

    int label_counter;

    // Anonymous functions collected from the AST
    AstNode* anon_fns[SE_MAX_FUNCTIONS];
    size_t anon_fn_count;

    bool needs_indirect_call; // True if any indirect calls are emitted

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
