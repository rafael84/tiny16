#include "ast.h"

#include <stdlib.h>
#include <string.h>

const char* ast_kind_name(AstKind kind) {
    switch (kind) {
    case AST_NUMBER: return "NUMBER";
    case AST_STRING: return "STRING";
    case AST_SYMBOL: return "SYMBOL";
    case AST_DEF: return "DEF";
    case AST_DEFN: return "DEFN";
    case AST_LET: return "LET";
    case AST_SET: return "SET";
    case AST_IF: return "IF";
    case AST_WHILE: return "WHILE";
    case AST_DO: return "DO";
    case AST_DATA: return "DATA";
    case AST_DB: return "DB";
    case AST_REPEAT: return "REPEAT";
    case AST_ADD: return "ADD";
    case AST_SUB: return "SUB";
    case AST_NEG: return "NEG";
    case AST_INC: return "INC";
    case AST_DEC: return "DEC";
    case AST_AND: return "AND";
    case AST_OR: return "OR";
    case AST_XOR: return "XOR";
    case AST_NOT: return "NOT";
    case AST_SHL: return "SHL";
    case AST_SHR: return "SHR";
    case AST_MUL: return "MUL";
    case AST_DIV: return "DIV";
    case AST_MOD: return "MOD";
    case AST_EQ: return "EQ";
    case AST_NE: return "NE";
    case AST_LT: return "LT";
    case AST_GT: return "GT";
    case AST_LE: return "LE";
    case AST_GE: return "GE";
    case AST_LNOT: return "LNOT";
    case AST_LOAD: return "LOAD";
    case AST_STORE: return "STORE";
    case AST_ADDR: return "ADDR";
    case AST_ADDR16: return "ADDR16";
    case AST_HI: return "HI";
    case AST_LO: return "LO";
    case AST_CALL: return "CALL";
    case AST_NS: return "NS";
    case AST_REQUIRE: return "REQUIRE";
    case AST_IMPORT: return "IMPORT";
    case AST_ASM: return "ASM";
    case AST_DEFMACRO: return "DEFMACRO";
    default: return "UNKNOWN";
    }
}

// Pool management
void ast_pool_init(AstPool* pool) {
    pool->chunks = NULL;
    pool->chunk_count = 0;
    pool->chunk_capacity = 0;
    pool->current_index = AST_CHUNK_SIZE; // Force allocation on first use
}

void ast_pool_free(AstPool* pool) {
    for (size_t i = 0; i < pool->chunk_count; i++) {
        free(pool->chunks[i]);
    }
    free(pool->chunks);
    pool->chunks = NULL;
    pool->chunk_count = 0;
    pool->chunk_capacity = 0;
    pool->current_index = AST_CHUNK_SIZE;
}

void ast_pool_reset(AstPool* pool) {
    // Reset to beginning of first chunk (keep allocated memory)
    pool->current_index = 0;
    if (pool->chunk_count > 0) {
        // Clear first chunk
        memset(pool->chunks[0], 0, AST_CHUNK_SIZE * sizeof(AstNode));
    }
    // Reset chunk count to 1 if we have any chunks
    if (pool->chunk_count > 1) {
        for (size_t i = 1; i < pool->chunk_count; i++) {
            free(pool->chunks[i]);
        }
        pool->chunk_count = 1;
    }
}

static bool ast_pool_grow(AstPool* pool) {
    // Grow chunks array if needed
    if (pool->chunk_count >= pool->chunk_capacity) {
        size_t new_cap = pool->chunk_capacity == 0 ? 4 : pool->chunk_capacity * 2;
        AstNode** new_chunks = realloc(pool->chunks, new_cap * sizeof(AstNode*));
        if (!new_chunks) return false;
        pool->chunks = new_chunks;
        pool->chunk_capacity = new_cap;
    }

    // Allocate new chunk
    AstNode* chunk = calloc(AST_CHUNK_SIZE, sizeof(AstNode));
    if (!chunk) return false;

    pool->chunks[pool->chunk_count++] = chunk;
    pool->current_index = 0;
    return true;
}

AstNode* ast_alloc(AstPool* pool) {
    // Need new chunk?
    if (pool->current_index >= AST_CHUNK_SIZE) {
        if (!ast_pool_grow(pool)) {
            return NULL;
        }
    }

    AstNode* node = &pool->chunks[pool->chunk_count - 1][pool->current_index++];
    memset(node, 0, sizeof(*node));
    return node;
}

// Array management
void ast_array_init(AstNodeArray* arr) {
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

bool ast_array_push(AstNodeArray* arr, AstNode* node) {
    if (arr->count >= arr->capacity) {
        size_t new_cap = arr->capacity == 0 ? 8 : arr->capacity * 2;
        AstNode** new_items = realloc(arr->items, new_cap * sizeof(AstNode*));
        if (!new_items) return false;
        arr->items = new_items;
        arr->capacity = new_cap;
    }
    arr->items[arr->count++] = node;
    return true;
}

void ast_array_free(AstNodeArray* arr) {
    free(arr->items);
    arr->items = NULL;
    arr->count = 0;
    arr->capacity = 0;
}

// Program management
void ast_program_init(AstProgram* prog) {
    prog->nodes = NULL;
    prog->node_count = 0;
    prog->node_capacity = 0;
}

bool ast_program_add(AstProgram* prog, AstNode* node) {
    if (prog->node_count >= prog->node_capacity) {
        size_t new_cap = prog->node_capacity == 0 ? 64 : prog->node_capacity * 2;
        AstNode** new_nodes = realloc(prog->nodes, new_cap * sizeof(AstNode*));
        if (!new_nodes) return false;
        prog->nodes = new_nodes;
        prog->node_capacity = new_cap;
    }
    prog->nodes[prog->node_count++] = node;
    return true;
}

void ast_program_free(AstProgram* prog) {
    free(prog->nodes);
    prog->nodes = NULL;
    prog->node_count = 0;
    prog->node_capacity = 0;
}
