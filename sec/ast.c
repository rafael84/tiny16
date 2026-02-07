#include "ast.h"

#include <stdlib.h>
#include <string.h>

const char* ast_kind_name(AstKind kind) {
    switch (kind) {
    case AST_NUMBER: return "NUMBER";
    case AST_STRING: return "STRING";
    case AST_SYMBOL: return "SYMBOL";
    case AST_KEYWORD: return "KEYWORD";
    case AST_NIL: return "NIL";
    case AST_TRUE: return "TRUE";
    case AST_FALSE: return "FALSE";
    case AST_DEF: return "DEF";
    case AST_DEFN: return "DEFN";
    case AST_LET: return "LET";
    case AST_SET: return "SET";
    case AST_SET_BANG: return "SET_BANG";
    case AST_IF: return "IF";
    case AST_WHILE: return "WHILE";
    case AST_DO: return "DO";
    case AST_COND: return "COND";
    case AST_WHEN: return "WHEN";
    case AST_UNLESS: return "UNLESS";
    case AST_FOR: return "FOR";
    case AST_RANGE: return "RANGE";
    case AST_DATA: return "DATA";
    case AST_DB: return "DB";
    case AST_REPEAT: return "REPEAT";
    case AST_ADD: return "ADD";
    case AST_SUB: return "SUB";
    case AST_NEG: return "NEG";
    case AST_INC: return "INC";
    case AST_DEC: return "DEC";
    case AST_BAND: return "BAND";
    case AST_BOR: return "BOR";
    case AST_XOR: return "XOR";
    case AST_BNOT: return "BNOT";
    case AST_SHL: return "SHL";
    case AST_SHR: return "SHR";
    case AST_MUL: return "MUL";
    case AST_DIV: return "DIV";
    case AST_MOD: return "MOD";
    case AST_LOGIC_AND: return "LOGIC_AND";
    case AST_LOGIC_OR: return "LOGIC_OR";
    case AST_LOGIC_NOT: return "LOGIC_NOT";
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
    case AST_DEFRECORD: return "DEFRECORD";
    case AST_FIELD_GET: return "FIELD_GET";
    case AST_ARRAY: return "ARRAY";
    case AST_NTH: return "NTH";
    case AST_LEN: return "LEN";
    case AST_NILP: return "NILP";
    case AST_ZEROP: return "ZEROP";
    case AST_POSP: return "POSP";
    case AST_NEGP: return "NEGP";
    case AST_FN: return "FN";
    case AST_CAST_U8: return "CAST_U8";
    case AST_CAST_I8: return "CAST_I8";
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

void ast_for_each_child(AstNode* node, AstChildCallback cb, void* ctx) {
    if (!node || !cb) return;

    switch (node->kind) {
    case AST_NUMBER:
    case AST_STRING:
    case AST_SYMBOL:
    case AST_KEYWORD:
    case AST_NIL:
    case AST_TRUE:
    case AST_FALSE:
    case AST_NS:
    case AST_IMPORT:
    case AST_ASM:
    case AST_DEFRECORD: break;

    case AST_DEF: cb(&node->as.def.value, ctx); break;

    case AST_DEFN:
    case AST_FN:
    case AST_DEFMACRO:
        for (size_t i = 0; i < node->as.defn.body.count; i++) {
            cb(&node->as.defn.body.items[i], ctx);
        }
        break;

    case AST_LET:
        for (size_t i = 0; i < node->as.let.binding_count; i++) {
            cb(&node->as.let.vals[i], ctx);
        }
        for (size_t i = 0; i < node->as.let.body.count; i++) {
            cb(&node->as.let.body.items[i], ctx);
        }
        break;

    case AST_SET:
    case AST_SET_BANG:
        cb(&node->as.set.value, ctx);
        if (node->as.set.target_expr) {
            cb(&node->as.set.target_expr, ctx);
        }
        break;

    case AST_VAR: cb(&node->as.var.value, ctx); break;

    case AST_COND:
        for (size_t i = 0; i < node->as.cond.clause_count; i++) {
            cb(&node->as.cond.tests[i], ctx);
            for (size_t j = 0; j < node->as.cond.bodies[i].count; j++) {
                cb(&node->as.cond.bodies[i].items[j], ctx);
            }
        }
        break;

    case AST_WHEN:
    case AST_UNLESS:
        cb(&node->as.when_expr.cond, ctx);
        for (size_t i = 0; i < node->as.when_expr.body.count; i++) {
            cb(&node->as.when_expr.body.items[i], ctx);
        }
        break;

    case AST_FOR:
        cb(&node->as.for_expr.collection, ctx);
        if (node->as.for_expr.when_cond) {
            cb(&node->as.for_expr.when_cond, ctx);
        }
        for (size_t i = 0; i < node->as.for_expr.body.count; i++) {
            cb(&node->as.for_expr.body.items[i], ctx);
        }
        break;

    case AST_RANGE:
        cb(&node->as.range.start, ctx);
        cb(&node->as.range.end, ctx);
        break;

    case AST_IF:
        cb(&node->as.if_expr.cond, ctx);
        cb(&node->as.if_expr.then_branch, ctx);
        cb(&node->as.if_expr.else_branch, ctx);
        break;

    case AST_WHILE:
        cb(&node->as.while_expr.cond, ctx);
        for (size_t i = 0; i < node->as.while_expr.body.count; i++) {
            cb(&node->as.while_expr.body.items[i], ctx);
        }
        break;

    case AST_DO:
    case AST_DB:
    case AST_REQUIRE:
        for (size_t i = 0; i < node->as.block.exprs.count; i++) {
            cb(&node->as.block.exprs.items[i], ctx);
        }
        break;

    case AST_DATA:
        if (node->as.data.addr_expr) {
            cb(&node->as.data.addr_expr, ctx);
        }
        for (size_t i = 0; i < node->as.data.body.count; i++) {
            cb(&node->as.data.body.items[i], ctx);
        }
        break;

    case AST_REPEAT: cb(&node->as.repeat.form, ctx); break;

    case AST_ADD:
    case AST_SUB:
    case AST_MUL:
    case AST_DIV:
    case AST_MOD:
    case AST_BAND:
    case AST_BOR:
    case AST_XOR:
    case AST_SHL:
    case AST_SHR:
    case AST_EQ:
    case AST_NE:
    case AST_LT:
    case AST_GT:
    case AST_LE:
    case AST_GE:
    case AST_LOGIC_AND:
    case AST_LOGIC_OR:
    case AST_NTH:
        cb(&node->as.binary.left, ctx);
        cb(&node->as.binary.right, ctx);
        break;

    case AST_NEG:
    case AST_INC:
    case AST_DEC:
    case AST_BNOT:
    case AST_LNOT:
    case AST_LOGIC_NOT:
    case AST_HI:
    case AST_LO:
    case AST_ADDR16:
    case AST_LEN:
    case AST_NILP:
    case AST_ZEROP:
    case AST_POSP:
    case AST_NEGP:
    case AST_CAST_U8:
    case AST_CAST_I8: cb(&node->as.unary.operand, ctx); break;

    case AST_LOAD: cb(&node->as.load.addr, ctx); break;

    case AST_STORE:
        cb(&node->as.store.addr, ctx);
        cb(&node->as.store.value, ctx);
        break;

    case AST_ADDR:
        cb(&node->as.addr.hi, ctx);
        cb(&node->as.addr.lo, ctx);
        break;

    case AST_CALL:
        for (size_t i = 0; i < node->as.call.arg_count; i++) {
            cb(&node->as.call.args[i], ctx);
        }
        break;

    case AST_FIELD_GET: cb(&node->as.field_get.record, ctx); break;

    case AST_ARRAY:
        cb(&node->as.array_expr.count, ctx);
        cb(&node->as.array_expr.value, ctx);
        break;

    default: break;
    }
}
