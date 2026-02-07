#include "macro.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void se_macro_init(SeMacroTable* table) { memset(table, 0, sizeof(*table)); }

static SeMacro* find_macro(SeMacroTable* table, const char* name) {
    for (size_t i = 0; i < table->macro_count; i++) {
        if (strcmp(table->macros[i].name, name) == 0) {
            return &table->macros[i];
        }
    }
    return NULL;
}

void se_macro_collect(SeMacroTable* table, AstProgram* program) {
    for (size_t i = 0; i < program->node_count; i++) {
        AstNode* node = program->nodes[i];
        if (node->kind == AST_DEFMACRO) {
            if (table->macro_count >= SE_MAX_MACROS) {
                fprintf(stderr, "error: too many macros (max %d)\n", SE_MAX_MACROS);
                return;
            }
            SeMacro* macro = &table->macros[table->macro_count++];
            strncpy(macro->name, node->as.defn.name, SE_MAX_SYMBOL_LEN - 1);
            macro->param_count = node->as.defn.param_count;
            for (size_t p = 0; p < macro->param_count; p++) {
                strncpy(macro->params[p], node->as.defn.params[p], SE_MAX_SYMBOL_LEN - 1);
            }
            macro->body = node->as.defn.body;
        }
    }
}

typedef struct {
    SeMacroTable* table;
    AstPool* pool;
    char params[SE_MAX_PARAMS][SE_MAX_SYMBOL_LEN];
    AstNode* args[SE_MAX_PARAMS];
    size_t param_count;
} MacroContext;

static AstNode* expand_node(MacroContext* ctx, AstNode* src);

static void subst_name(MacroContext* ctx, char* dest, const char* src) {
    const char* p = src;
    size_t out = 0;

    while (*p && out < SE_MAX_SYMBOL_LEN - 1) {
        if (*p == '{') {
            const char* end = strchr(p, '}');
            if (end) {
                char param[SE_MAX_SYMBOL_LEN];
                size_t len = end - p - 1;
                if (len < SE_MAX_SYMBOL_LEN) {
                    memcpy(param, p + 1, len);
                    param[len] = '\0';
                    for (size_t i = 0; i < ctx->param_count; i++) {
                        if (strcmp(param, ctx->params[i]) == 0) {
                            if (ctx->args[i]->kind == AST_SYMBOL) {
                                size_t arg_len = strlen(ctx->args[i]->as.symbol.name);
                                if (out + arg_len < SE_MAX_SYMBOL_LEN) {
                                    strcpy(dest + out, ctx->args[i]->as.symbol.name);
                                    out += arg_len;
                                }
                            } else if (ctx->args[i]->kind == AST_NUMBER) {
                                char num_str[32];
                                snprintf(num_str, sizeof(num_str), "%d", ctx->args[i]->as.number);
                                size_t arg_len = strlen(num_str);
                                if (out + arg_len < SE_MAX_SYMBOL_LEN) {
                                    strcpy(dest + out, num_str);
                                    out += arg_len;
                                }
                            }
                            break;
                        }
                    }
                    p = end + 1;
                    continue;
                }
            }
        }
        dest[out++] = *p++;
    }
    dest[out] = '\0';
}

static void expand_array(MacroContext* ctx, AstNodeArray* dest, AstNodeArray* src) {
    ast_array_init(dest);
    for (size_t i = 0; i < src->count; i++) {
        AstNode* expanded = expand_node(ctx, src->items[i]);
        if (expanded) ast_array_push(dest, expanded);
    }
}

static AstNode* expand_node(MacroContext* ctx, AstNode* src) {
    if (!src) return NULL;

    if (src->kind == AST_CALL) {
        SeMacro* macro = find_macro(ctx->table, src->as.call.func);
        if (macro) {
            if (src->as.call.arg_count != macro->param_count) {
                fprintf(stderr, "error: macro '%s' expects %zu args, got %zu\n", macro->name,
                        macro->param_count, src->as.call.arg_count);
                return src;
            }

            MacroContext nested;
            nested.table = ctx->table;
            nested.pool = ctx->pool;
            nested.param_count = macro->param_count;
            for (size_t i = 0; i < macro->param_count; i++) {
                strncpy(nested.params[i], macro->params[i], SE_MAX_SYMBOL_LEN - 1);
                nested.args[i] = src->as.call.args[i];
            }

            if (macro->body.count == 1) {
                return expand_node(&nested, macro->body.items[0]);
            } else {
                AstNode* do_node = ast_alloc(ctx->pool);
                if (!do_node) return src;
                do_node->kind = AST_DO;
                do_node->line = src->line;
                do_node->column = src->column;
                expand_array(&nested, &do_node->as.block.exprs, &macro->body);
                return do_node;
            }
        }
    }

    if (src->kind == AST_SYMBOL) {
        const char* name = src->as.symbol.name;

        if (name[0] == '{' && name[strlen(name) - 1] == '}') {
            char param[SE_MAX_SYMBOL_LEN];
            size_t len = strlen(name) - 2;
            memcpy(param, name + 1, len);
            param[len] = '\0';

            for (size_t i = 0; i < ctx->param_count; i++) {
                if (strcmp(param, ctx->params[i]) == 0) {
                    return expand_node(ctx, ctx->args[i]);
                }
            }
        }

        if (strchr(name, '{')) {
            char substituted[SE_MAX_SYMBOL_LEN];
            subst_name(ctx, substituted, name);
            char* end;
            long num = strtol(substituted, &end, 0);
            if (*end == '\0') {
                AstNode* node = ast_alloc(ctx->pool);
                if (!node) return NULL;
                node->kind = AST_NUMBER;
                node->line = src->line;
                node->column = src->column;
                node->as.number = (int32_t)num;
                return node;
            }
            AstNode* node = ast_alloc(ctx->pool);
            if (!node) return NULL;
            *node = *src;
            strncpy(node->as.symbol.name, substituted, SE_MAX_SYMBOL_LEN - 1);
            return node;
        }

        for (size_t i = 0; i < ctx->param_count; i++) {
            if (strcmp(name, ctx->params[i]) == 0) {
                return expand_node(ctx, ctx->args[i]);
            }
        }
    }

    AstNode* node = ast_alloc(ctx->pool);
    if (!node) return NULL;
    *node = *src;

    switch (src->kind) {
    case AST_DEF:
        subst_name(ctx, node->as.def.name, src->as.def.name);
        node->as.def.value = expand_node(ctx, src->as.def.value);
        break;

    case AST_DEFN:
    case AST_DEFMACRO:
    case AST_FN: expand_array(ctx, &node->as.defn.body, &src->as.defn.body); break;

    case AST_LET:
        for (size_t i = 0; i < src->as.let.binding_count; i++) {
            subst_name(ctx, node->as.let.vars[i], src->as.let.vars[i]);
            node->as.let.vals[i] = expand_node(ctx, src->as.let.vals[i]);
        }
        expand_array(ctx, &node->as.let.body, &src->as.let.body);
        break;

    case AST_SET:
    case AST_SET_BANG:
        subst_name(ctx, node->as.set.var, src->as.set.var);
        node->as.set.value = expand_node(ctx, src->as.set.value);
        if (src->as.set.target_expr) {
            node->as.set.target_expr = expand_node(ctx, src->as.set.target_expr);
        }
        break;

    case AST_VAR:
        subst_name(ctx, node->as.var.name, src->as.var.name);
        node->as.var.value = expand_node(ctx, src->as.var.value);
        break;

    case AST_COND:
        node->as.cond.clause_count = src->as.cond.clause_count;
        for (size_t i = 0; i < src->as.cond.clause_count; i++) {
            node->as.cond.tests[i] = expand_node(ctx, src->as.cond.tests[i]);
            expand_array(ctx, &node->as.cond.bodies[i], &src->as.cond.bodies[i]);
        }
        break;

    case AST_WHEN:
    case AST_UNLESS:
        node->as.when_expr.cond = expand_node(ctx, src->as.when_expr.cond);
        expand_array(ctx, &node->as.when_expr.body, &src->as.when_expr.body);
        break;

    case AST_FOR:
        subst_name(ctx, node->as.for_expr.var, src->as.for_expr.var);
        node->as.for_expr.collection = expand_node(ctx, src->as.for_expr.collection);
        node->as.for_expr.when_cond =
            src->as.for_expr.when_cond ? expand_node(ctx, src->as.for_expr.when_cond) : NULL;
        expand_array(ctx, &node->as.for_expr.body, &src->as.for_expr.body);
        break;

    case AST_RANGE:
        node->as.range.start = expand_node(ctx, src->as.range.start);
        node->as.range.end = expand_node(ctx, src->as.range.end);
        break;

    case AST_LOGIC_AND:
    case AST_LOGIC_OR:
        node->as.binary.left = expand_node(ctx, src->as.binary.left);
        node->as.binary.right = expand_node(ctx, src->as.binary.right);
        break;

    case AST_LOGIC_NOT: node->as.unary.operand = expand_node(ctx, src->as.unary.operand); break;

    case AST_IF:
        node->as.if_expr.cond = expand_node(ctx, src->as.if_expr.cond);
        node->as.if_expr.then_branch = expand_node(ctx, src->as.if_expr.then_branch);
        node->as.if_expr.else_branch = expand_node(ctx, src->as.if_expr.else_branch);
        break;

    case AST_WHILE:
        node->as.while_expr.cond = expand_node(ctx, src->as.while_expr.cond);
        expand_array(ctx, &node->as.while_expr.body, &src->as.while_expr.body);
        break;

    case AST_DO:
    case AST_DB:
    case AST_REQUIRE: expand_array(ctx, &node->as.block.exprs, &src->as.block.exprs); break;

    case AST_DATA:
        subst_name(ctx, node->as.data.name, src->as.data.name);
        node->as.data.addr_expr = expand_node(ctx, src->as.data.addr_expr);
        expand_array(ctx, &node->as.data.body, &src->as.data.body);
        break;

    case AST_REPEAT: node->as.repeat.form = expand_node(ctx, src->as.repeat.form); break;

    case AST_ADD:
    case AST_SUB:
    case AST_BAND:
    case AST_BOR:
    case AST_XOR:
    case AST_SHL:
    case AST_SHR:
    case AST_MUL:
    case AST_DIV:
    case AST_MOD:
    case AST_EQ:
    case AST_NE:
    case AST_LT:
    case AST_GT:
    case AST_LE:
    case AST_GE:
        node->as.binary.left = expand_node(ctx, src->as.binary.left);
        node->as.binary.right = expand_node(ctx, src->as.binary.right);
        break;

    case AST_NEG:
    case AST_INC:
    case AST_DEC:
    case AST_BNOT:
    case AST_LNOT:
    case AST_HI:
    case AST_LO:
    case AST_ADDR16: node->as.unary.operand = expand_node(ctx, src->as.unary.operand); break;

    case AST_LOAD: node->as.load.addr = expand_node(ctx, src->as.load.addr); break;

    case AST_STORE:
        node->as.store.addr = expand_node(ctx, src->as.store.addr);
        node->as.store.value = expand_node(ctx, src->as.store.value);
        break;

    case AST_ADDR:
        node->as.addr.hi = expand_node(ctx, src->as.addr.hi);
        node->as.addr.lo = expand_node(ctx, src->as.addr.lo);
        break;

    case AST_CALL:
        subst_name(ctx, node->as.call.func, src->as.call.func);
        for (size_t i = 0; i < src->as.call.arg_count; i++) {
            node->as.call.args[i] = expand_node(ctx, src->as.call.args[i]);
        }
        break;

    case AST_FIELD_GET:
        subst_name(ctx, node->as.field_get.field, src->as.field_get.field);
        node->as.field_get.record = expand_node(ctx, src->as.field_get.record);
        break;

    case AST_DEFRECORD: break; // Nothing to expand in record definitions

    case AST_ARRAY:
        node->as.array_expr.count = expand_node(ctx, src->as.array_expr.count);
        node->as.array_expr.value = expand_node(ctx, src->as.array_expr.value);
        break;

    case AST_NTH:
        node->as.binary.left = expand_node(ctx, src->as.binary.left);
        node->as.binary.right = expand_node(ctx, src->as.binary.right);
        break;

    case AST_LEN:
    case AST_NILP:
    case AST_ZEROP:
    case AST_POSP:
    case AST_NEGP:
    case AST_CAST_U8:
    case AST_CAST_I8: node->as.unary.operand = expand_node(ctx, src->as.unary.operand); break;

    default: break;
    }

    return node;
}

void se_macro_expand(SeMacroTable* table, AstProgram* program, AstPool* pool) {
    MacroContext ctx;
    ctx.table = table;
    ctx.pool = pool;
    ctx.param_count = 0;
    for (size_t i = 0; i < program->node_count; i++) {
        if (program->nodes[i]->kind != AST_DEFMACRO) {
            program->nodes[i] = expand_node(&ctx, program->nodes[i]);
        }
    }
}
