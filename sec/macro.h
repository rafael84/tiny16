#pragma once

#include "ast.h"

#include <stdbool.h>

#define SE_MAX_MACROS 64

typedef struct {
    char name[SE_MAX_SYMBOL_LEN];
    char params[SE_MAX_PARAMS][SE_MAX_SYMBOL_LEN];
    size_t param_count;
    AstNodeArray body;
} SeMacro;

typedef struct {
    SeMacro macros[SE_MAX_MACROS];
    size_t macro_count;
} SeMacroTable;

void se_macro_init(SeMacroTable* table);
void se_macro_collect(SeMacroTable* table, AstProgram* program);
void se_macro_expand(SeMacroTable* table, AstProgram* program, AstPool* pool);
