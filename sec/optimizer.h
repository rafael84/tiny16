#pragma once

#include "ast.h"

typedef enum {
    SE_OPT_NONE = 0, // -O0: no optimization
    SE_OPT_BASIC,    // -O1: constant-fold, constant-prop, dead-code, keyword-intern
    SE_OPT_FULL,     // -O2: all passes
} SeOptLevel;

// Run optimization passes on the AST program at the given level.
// pool is used for allocating new AST nodes during transformations.
void se_optimize(AstProgram* program, AstPool* pool, SeOptLevel level);
