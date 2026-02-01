#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SE_MAX_CHILDREN   64
#define SE_MAX_PARAMS     16
#define SE_MAX_FUNCTIONS  128
#define SE_MAX_CONSTANTS  256
#define SE_MAX_SYMBOL_LEN 64

typedef enum {
    // Atoms
    AST_NUMBER, // literal number
    AST_STRING, // literal string
    AST_SYMBOL, // identifier reference

    // Special forms
    AST_DEF,    // (def name value)
    AST_DEFN,   // (defn name (params) body...)
    AST_LET,    // (let (bindings) body...)
    AST_SET,    // (set var value)
    AST_IF,     // (if cond then else)
    AST_WHILE,  // (while cond body...)
    AST_DO,     // (do expr...)
    AST_DATA,   // (data name [addr] body...)
    AST_DB,     // (db values...)
    AST_REPEAT, // (repeat count form)

    // Primitives
    AST_ADD, // (+ a b) or (add a b)
    AST_SUB, // (- a b) or (sub a b)
    AST_NEG, // (- a) or (neg a)
    AST_INC, // (inc a)
    AST_DEC, // (dec a)
    AST_AND, // (& a b) or (and a b)
    AST_OR,  // (| a b) or (or a b)
    AST_XOR, // (^ a b) or (xor a b)
    AST_NOT, // (~ a) or (not a)
    AST_SHL, // (<< a b) or (shl a b)
    AST_SHR, // (>> a b) or (shr a b)
    AST_MUL, // (* a b) or (mul a b)
    AST_DIV, // (/ a b) or (div a b)
    AST_MOD, // (% a b) or (mod a b)

    // Comparisons
    AST_EQ,   // (= a b) or (eq a b)
    AST_NE,   // (!= a b) or (ne a b)
    AST_LT,   // (< a b) or (lt a b)
    AST_GT,   // (> a b) or (gt a b)
    AST_LE,   // (<= a b) or (le a b)
    AST_GE,   // (>= a b) or (ge a b)
    AST_LNOT, // (! a) or (lnot a)

    // Memory
    AST_LOAD,   // (load addr-expr)
    AST_STORE,  // (store addr-expr value)
    AST_ADDR,   // (addr hi lo)
    AST_ADDR16, // (addr16 value) - 16-bit address, auto-splits to hi/lo

    // Compile-time helpers
    AST_HI, // (hi addr)
    AST_LO, // (lo addr)

    // Function call
    AST_CALL, // (func args...)

    // Special directives
    AST_INCLUDE, // (include "filename")
    AST_ASM,     // (asm "raw assembly...")
} AstKind;

const char* ast_kind_name(AstKind kind);

// Forward declaration
typedef struct AstNode AstNode;

struct AstNode {
    AstKind kind;
    size_t line;
    size_t column;

    union {
        // AST_NUMBER
        int32_t number;

        // AST_STRING, AST_SYMBOL
        struct {
            char name[SE_MAX_SYMBOL_LEN];
        } symbol;

        // AST_DEF: (def name value)
        struct {
            char name[SE_MAX_SYMBOL_LEN];
            AstNode* value;
        } def;

        // AST_DEFN: (defn name (params) body...)
        struct {
            char name[SE_MAX_SYMBOL_LEN];
            char params[SE_MAX_PARAMS][SE_MAX_SYMBOL_LEN];
            size_t param_count;
            AstNode* body[SE_MAX_CHILDREN];
            size_t body_count;
        } defn;

        // AST_LET: (let (bindings) body...)
        struct {
            char vars[SE_MAX_PARAMS][SE_MAX_SYMBOL_LEN];
            AstNode* vals[SE_MAX_PARAMS];
            size_t binding_count;
            AstNode* body[SE_MAX_CHILDREN];
            size_t body_count;
        } let;

        // AST_SET: (set var value)
        struct {
            char var[SE_MAX_SYMBOL_LEN];
            AstNode* value;
        } set;

        // AST_IF: (if cond then else)
        struct {
            AstNode* cond;
            AstNode* then_branch;
            AstNode* else_branch;
        } if_expr;

        // AST_WHILE: (while cond body...)
        struct {
            AstNode* cond;
            AstNode* body[SE_MAX_CHILDREN];
            size_t body_count;
        } while_expr;

        // AST_DO: (do expr...), also used for multi-arg ops
        struct {
            AstNode* exprs[SE_MAX_CHILDREN];
            size_t expr_count;
        } block;

        // AST_DATA: (data name addr body...)
        struct {
            char name[SE_MAX_SYMBOL_LEN];
            int32_t addr;       // -1 if not specified or needs evaluation
            AstNode* addr_expr; // expression for address (evaluated at codegen time)
            AstNode* body[SE_MAX_CHILDREN];
            size_t body_count;
        } data;

        // AST_REPEAT: (repeat count form)
        struct {
            int32_t count;
            AstNode* form;
        } repeat;

        // Binary ops: AST_ADD, AST_SUB, etc.
        struct {
            AstNode* left;
            AstNode* right;
        } binary;

        // Unary ops: AST_NEG, AST_NOT, AST_INC, AST_DEC, AST_LNOT
        struct {
            AstNode* operand;
        } unary;

        // AST_LOAD: (load addr-expr)
        struct {
            AstNode* addr;
        } load;

        // AST_STORE: (store addr-expr value)
        struct {
            AstNode* addr;
            AstNode* value;
        } store;

        // AST_ADDR: (addr hi lo)
        struct {
            AstNode* hi;
            AstNode* lo;
        } addr;

        // AST_CALL: (func args...)
        struct {
            char func[SE_MAX_SYMBOL_LEN];
            AstNode* args[SE_MAX_PARAMS];
            size_t arg_count;
        } call;
    } as;
};

typedef struct {
    AstNode* nodes[SE_MAX_FUNCTIONS + SE_MAX_CONSTANTS];
    size_t node_count;
} AstProgram;

typedef struct {
    AstNode nodes[4096];
    size_t count;
} AstPool;

AstNode* ast_alloc(AstPool* pool);
void ast_pool_reset(AstPool* pool);
