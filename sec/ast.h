#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Compiler limits
#define SE_MAX_PARAMS     32 // Max function parameters (fixed, small)
#define SE_MAX_SYMBOL_LEN 64 // Max symbol name length

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
    AST_NS,       // (ns name)
    AST_REQUIRE,  // (require name...)
    AST_IMPORT,   // (import "filename")
    AST_ASM,      // (asm "raw assembly...")
    AST_DEFMACRO, // (defmacro name (params) body...)
} AstKind;

const char* ast_kind_name(AstKind kind);

// Forward declaration
typedef struct AstNode AstNode;

// Dynamic array of AstNode pointers
typedef struct {
    AstNode** items;
    size_t count;
    size_t capacity;
} AstNodeArray;

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
        // AST_DEFMACRO: (defmacro name (params) body...)
        struct {
            char name[SE_MAX_SYMBOL_LEN];
            char params[SE_MAX_PARAMS][SE_MAX_SYMBOL_LEN];
            size_t param_count;
            AstNodeArray body;
        } defn;

        // AST_LET: (let (bindings) body...)
        struct {
            char vars[SE_MAX_PARAMS][SE_MAX_SYMBOL_LEN];
            AstNode* vals[SE_MAX_PARAMS];
            size_t binding_count;
            AstNodeArray body;
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
            AstNodeArray body;
        } while_expr;

        // AST_DO: (do expr...), AST_DB, AST_REQUIRE
        struct {
            AstNodeArray exprs;
        } block;

        // AST_DATA: (data name addr body...)
        struct {
            char name[SE_MAX_SYMBOL_LEN];
            int32_t addr;       // -1 if not specified or needs evaluation
            AstNode* addr_expr; // expression for address (evaluated at codegen time)
            AstNodeArray body;
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

// Dynamic program structure
typedef struct {
    AstNode** nodes;
    size_t node_count;
    size_t node_capacity;
} AstProgram;

// Dynamic AST node pool
typedef struct {
    AstNode** chunks; // Array of chunk pointers
    size_t chunk_count;
    size_t chunk_capacity;
    size_t current_index; // Index within current chunk
} AstPool;

#define AST_CHUNK_SIZE 1024 // Nodes per chunk

// Pool management
void ast_pool_init(AstPool* pool);
void ast_pool_free(AstPool* pool);
AstNode* ast_alloc(AstPool* pool);

// Array management
void ast_array_init(AstNodeArray* arr);
bool ast_array_push(AstNodeArray* arr, AstNode* node);
void ast_array_free(AstNodeArray* arr);

// Program management
void ast_program_init(AstProgram* prog);
bool ast_program_add(AstProgram* prog, AstNode* node);
void ast_program_free(AstProgram* prog);

// Legacy compatibility
void ast_pool_reset(AstPool* pool);
