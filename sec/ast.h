#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Compiler limits
#define SE_MAX_PARAMS     32 // Max function parameters (fixed, small)
#define SE_MAX_SYMBOL_LEN 64 // Max symbol name length

#define SE_MAX_COND_CLAUSES 32

// Type hints for variables, parameters, and let bindings
typedef enum {
    SE_HINT_NONE = 0, // No hint - infer from value
    SE_HINT_U8,       // ^u8
    SE_HINT_I8,       // ^i8
    SE_HINT_U16,      // ^u16
    SE_HINT_I16,      // ^i16
} SeTypeHint;

static inline bool se_hint_is_16bit(SeTypeHint hint) {
    return hint == SE_HINT_U16 || hint == SE_HINT_I16;
}

static inline bool se_hint_is_signed(SeTypeHint hint) {
    return hint == SE_HINT_I8 || hint == SE_HINT_I16;
}

typedef enum {
    // Atoms
    AST_NUMBER,  // literal number
    AST_STRING,  // literal string
    AST_SYMBOL,  // identifier reference
    AST_KEYWORD, // :alive, :x
    AST_NIL,     // nil
    AST_TRUE,    // true
    AST_FALSE,   // false

    // Special forms
    AST_DEF,      // (def name value)
    AST_DEFN,     // (defn name (params) body...)
    AST_LET,      // (let (bindings) body...)
    AST_SET,      // (set var value) - legacy, unused in v2
    AST_SET_BANG, // (set! target value)
    AST_VAR,      // (var name value)
    AST_IF,       // (if cond then else)
    AST_WHILE,    // (while cond body...)
    AST_DO,       // (do expr...)
    AST_COND,     // (cond (test body...)...)
    AST_WHEN,     // (when cond body...)
    AST_UNLESS,   // (unless cond body...)
    AST_FOR,      // (for (binding collection) body...) or (for (binding coll :when cond) body...)
    AST_RANGE,    // (range start end)
    AST_DATA,     // (data name [addr] body...) - legacy, unused in v2
    AST_DB,       // (db values...) - legacy
    AST_REPEAT,   // (repeat count form) - legacy

    // Primitives - bitwise
    AST_BAND, // (& a b)
    AST_BOR,  // (| a b)
    AST_XOR,  // (^ a b)
    AST_BNOT, // (~ a)
    // Primitives - arithmetic
    AST_ADD, // (+ a b)
    AST_SUB, // (- a b)
    AST_NEG, // (- a)
    AST_INC, // (inc a)
    AST_DEC, // (dec a)
    AST_SHL, // (<< a b)
    AST_SHR, // (>> a b)
    AST_MUL, // (* a b)
    AST_DIV, // (/ a b)
    AST_MOD, // (% a b)

    // Logical (short-circuit)
    AST_LOGIC_AND, // (and expr...)
    AST_LOGIC_OR,  // (or expr...)
    AST_LOGIC_NOT, // (not expr)

    // Comparisons
    AST_EQ,   // (= a b)
    AST_NE,   // (!= a b)
    AST_LT,   // (< a b)
    AST_GT,   // (> a b)
    AST_LE,   // (<= a b)
    AST_GE,   // (>= a b)
    AST_LNOT, // (! a) - legacy logical not

    // Memory
    AST_LOAD,   // (load addr)
    AST_STORE,  // (store addr value)
    AST_ADDR,   // (addr hi lo) - legacy
    AST_ADDR16, // (addr16 value) - legacy

    // Compile-time helpers
    AST_HI, // (hi addr)
    AST_LO, // (lo addr)

    // Function call
    AST_CALL, // (func args...)

    // Special directives
    AST_NS,        // (ns name)
    AST_REQUIRE,   // (require name...)
    AST_IMPORT,    // (import "filename")
    AST_ASM,       // (asm "raw assembly...")
    AST_DEFMACRO,  // (defmacro name (params) body...)
    AST_DEFRECORD, // (defrecord name (fields...))
    AST_FIELD_GET, // (:field record) - keyword accessor read
    AST_ARRAY,     // (array count value) - fixed-size array
    AST_NTH,       // (nth array index) - element access
    AST_LEN,       // (len array) - array length (compile-time)

    // Type predicates (unary, return true/false)
    AST_NILP,  // (nil? x) - true if x is nil (0xFF)
    AST_ZEROP, // (zero? x) - true if x is 0
    AST_POSP,  // (pos? x) - true if x > 0 (signed)
    AST_NEGP,  // (neg? x) - true if x < 0 (signed, bit 7 set)

    // Anonymous function
    AST_FN, // (fn (params) body...) - anonymous function, reuses defn layout

    // Type casts (unary)
    AST_CAST_U8, // (u8 expr) - truncate to 8-bit unsigned
    AST_CAST_I8, // (i8 expr) - truncate to 8-bit signed
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
            SeTypeHint param_hints[SE_MAX_PARAMS];
            size_t param_count;
            AstNodeArray body;
        } defn;

        // AST_LET: (let (bindings) body...)
        struct {
            char vars[SE_MAX_PARAMS][SE_MAX_SYMBOL_LEN];
            AstNode* vals[SE_MAX_PARAMS];
            SeTypeHint hints[SE_MAX_PARAMS];
            size_t binding_count;
            AstNodeArray body;
        } let;

        // AST_SET / AST_SET_BANG: (set var value) or (set! target value)
        // For (set! (:field rec) value): var = ":field", target_expr = rec, value = value
        struct {
            char var[SE_MAX_SYMBOL_LEN];
            AstNode* value;
            AstNode* target_expr; // NULL for simple var, non-NULL for (:field record)
        } set;

        // AST_VAR: (var name value)
        struct {
            char name[SE_MAX_SYMBOL_LEN];
            AstNode* value;
            SeTypeHint type_hint;
        } var;

        // AST_COND: (cond (test body...)...)
        struct {
            AstNode* tests[SE_MAX_COND_CLAUSES];
            AstNodeArray bodies[SE_MAX_COND_CLAUSES];
            size_t clause_count;
        } cond;

        // AST_WHEN, AST_UNLESS: (when cond body...) - same layout as while_expr
        struct {
            AstNode* cond;
            AstNodeArray body;
        } when_expr;

        // AST_FOR: (for (binding collection) body...) or with :when
        struct {
            char var[SE_MAX_SYMBOL_LEN];
            AstNode* collection;
            AstNode* when_cond; /* NULL if no :when */
            AstNodeArray body;
        } for_expr;

        // AST_RANGE: (range start end)
        struct {
            AstNode* start;
            AstNode* end;
        } range;

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

        // AST_DEFRECORD: (defrecord name (fields...))
        struct {
            char name[SE_MAX_SYMBOL_LEN];
            char fields[SE_MAX_PARAMS][SE_MAX_SYMBOL_LEN];
            bool field_is_16bit[SE_MAX_PARAMS];  // true if field has ^i16 or ^u16 hint
            bool field_is_signed[SE_MAX_PARAMS]; // true if field has ^i16 or ^i8 hint
            size_t field_count;
        } defrecord;

        // AST_FIELD_GET: (:field record-expr)
        struct {
            char field[SE_MAX_SYMBOL_LEN]; // keyword name including ':'
            AstNode* record;               // record expression
        } field_get;

        // AST_ARRAY: (array count value)
        struct {
            AstNode* count; // compile-time constant count
            AstNode* value; // initial value or record constructor
        } array_expr;

        // AST_NTH: (nth array-expr index-expr)
        // Reuses binary: left = array, right = index

        // AST_LEN: (len array-expr)
        // Reuses unary: operand = array
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
