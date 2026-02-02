#include "ast.h"

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
    default: return "UNKNOWN";
    }
}

AstNode* ast_alloc(AstPool* pool) {
    if (pool->count >= sizeof(pool->nodes) / sizeof(pool->nodes[0])) {
        return NULL;
    }
    AstNode* node = &pool->nodes[pool->count++];
    memset(node, 0, sizeof(*node));
    return node;
}

void ast_pool_reset(AstPool* pool) { pool->count = 0; }
