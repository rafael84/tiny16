#include "optimizer.h"

#include <stdbool.h>
#include <string.h>

// -------------------------------------------------------------------
// Constant folding – evaluate constant expressions at compile time
// -------------------------------------------------------------------

static bool is_const_number(AstNode* n) { return n && n->kind == AST_NUMBER; }

static AstNode* make_number(AstPool* pool, int32_t value, size_t line, size_t col) {
    AstNode* n = ast_alloc(pool);
    if (!n) return NULL;
    n->kind = AST_NUMBER;
    n->line = line;
    n->column = col;
    n->as.number = value;
    return n;
}

// Forward
static AstNode* fold_node(AstPool* pool, AstNode* node);

static void fold_array(AstPool* pool, AstNodeArray* arr) {
    for (size_t i = 0; i < arr->count; i++) {
        arr->items[i] = fold_node(pool, arr->items[i]);
    }
}

static AstNode* fold_node(AstPool* pool, AstNode* node) {
    if (!node) return NULL;

    // Recurse into children first
    switch (node->kind) {
    case AST_DEF: node->as.def.value = fold_node(pool, node->as.def.value); break;
    case AST_DEFN:
    case AST_FN:
    case AST_DEFMACRO: fold_array(pool, &node->as.defn.body); break;
    case AST_LET:
        for (size_t i = 0; i < node->as.let.binding_count; i++)
            node->as.let.vals[i] = fold_node(pool, node->as.let.vals[i]);
        fold_array(pool, &node->as.let.body);
        break;
    case AST_VAR: node->as.var.value = fold_node(pool, node->as.var.value); break;
    case AST_SET:
    case AST_SET_BANG:
        node->as.set.value = fold_node(pool, node->as.set.value);
        if (node->as.set.target_expr)
            node->as.set.target_expr = fold_node(pool, node->as.set.target_expr);
        break;
    case AST_IF:
        node->as.if_expr.cond = fold_node(pool, node->as.if_expr.cond);
        node->as.if_expr.then_branch = fold_node(pool, node->as.if_expr.then_branch);
        node->as.if_expr.else_branch = fold_node(pool, node->as.if_expr.else_branch);
        break;
    case AST_WHILE:
        node->as.while_expr.cond = fold_node(pool, node->as.while_expr.cond);
        fold_array(pool, &node->as.while_expr.body);
        break;
    case AST_COND:
        for (size_t i = 0; i < node->as.cond.clause_count; i++) {
            node->as.cond.tests[i] = fold_node(pool, node->as.cond.tests[i]);
            fold_array(pool, &node->as.cond.bodies[i]);
        }
        break;
    case AST_WHEN:
    case AST_UNLESS:
        node->as.when_expr.cond = fold_node(pool, node->as.when_expr.cond);
        fold_array(pool, &node->as.when_expr.body);
        break;
    case AST_FOR:
        node->as.for_expr.collection = fold_node(pool, node->as.for_expr.collection);
        if (node->as.for_expr.when_cond)
            node->as.for_expr.when_cond = fold_node(pool, node->as.for_expr.when_cond);
        fold_array(pool, &node->as.for_expr.body);
        break;
    case AST_RANGE:
        node->as.range.start = fold_node(pool, node->as.range.start);
        node->as.range.end = fold_node(pool, node->as.range.end);
        break;
    case AST_DO:
    case AST_DB:
    case AST_REQUIRE: fold_array(pool, &node->as.block.exprs); break;
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
        node->as.binary.left = fold_node(pool, node->as.binary.left);
        node->as.binary.right = fold_node(pool, node->as.binary.right);
        break;
    case AST_NEG:
    case AST_INC:
    case AST_DEC:
    case AST_BNOT:
    case AST_LNOT:
    case AST_LOGIC_NOT:
    case AST_HI:
    case AST_LO:
    case AST_LEN:
    case AST_NILP:
    case AST_ZEROP:
    case AST_POSP:
    case AST_NEGP:
    case AST_CAST_U8:
    case AST_CAST_I8: node->as.unary.operand = fold_node(pool, node->as.unary.operand); break;
    case AST_CALL:
        for (size_t i = 0; i < node->as.call.arg_count; i++)
            node->as.call.args[i] = fold_node(pool, node->as.call.args[i]);
        break;
    case AST_LOAD: node->as.load.addr = fold_node(pool, node->as.load.addr); break;
    case AST_STORE:
        node->as.store.addr = fold_node(pool, node->as.store.addr);
        node->as.store.value = fold_node(pool, node->as.store.value);
        break;
    case AST_FIELD_GET:
        node->as.field_get.record = fold_node(pool, node->as.field_get.record);
        break;
    case AST_ARRAY:
        node->as.array_expr.count = fold_node(pool, node->as.array_expr.count);
        node->as.array_expr.value = fold_node(pool, node->as.array_expr.value);
        break;
    default: break;
    }

    // Now try to fold this node
    switch (node->kind) {
    case AST_ADD:
        if (is_const_number(node->as.binary.left) && is_const_number(node->as.binary.right)) {
            return make_number(pool,
                               node->as.binary.left->as.number + node->as.binary.right->as.number,
                               node->line, node->column);
        }
        // Identity: (+ x 0) -> x, (+ 0 x) -> x
        if (is_const_number(node->as.binary.right) && node->as.binary.right->as.number == 0)
            return node->as.binary.left;
        if (is_const_number(node->as.binary.left) && node->as.binary.left->as.number == 0)
            return node->as.binary.right;
        break;
    case AST_SUB:
        if (is_const_number(node->as.binary.left) && is_const_number(node->as.binary.right)) {
            return make_number(pool,
                               node->as.binary.left->as.number - node->as.binary.right->as.number,
                               node->line, node->column);
        }
        // Identity: (- x 0) -> x
        if (is_const_number(node->as.binary.right) && node->as.binary.right->as.number == 0)
            return node->as.binary.left;
        break;
    case AST_MUL:
        if (is_const_number(node->as.binary.left) && is_const_number(node->as.binary.right)) {
            return make_number(pool,
                               node->as.binary.left->as.number * node->as.binary.right->as.number,
                               node->line, node->column);
        }
        // Identity: (* x 1) -> x, (* 1 x) -> x
        if (is_const_number(node->as.binary.right) && node->as.binary.right->as.number == 1)
            return node->as.binary.left;
        if (is_const_number(node->as.binary.left) && node->as.binary.left->as.number == 1)
            return node->as.binary.right;
        // Annihilation: (* x 0) -> 0, (* 0 x) -> 0
        if (is_const_number(node->as.binary.right) && node->as.binary.right->as.number == 0)
            return make_number(pool, 0, node->line, node->column);
        if (is_const_number(node->as.binary.left) && node->as.binary.left->as.number == 0)
            return make_number(pool, 0, node->line, node->column);
        break;
    case AST_DIV:
        if (is_const_number(node->as.binary.left) && is_const_number(node->as.binary.right) &&
            node->as.binary.right->as.number != 0) {
            return make_number(pool,
                               node->as.binary.left->as.number / node->as.binary.right->as.number,
                               node->line, node->column);
        }
        // Identity: (/ x 1) -> x
        if (is_const_number(node->as.binary.right) && node->as.binary.right->as.number == 1)
            return node->as.binary.left;
        break;
    case AST_MOD:
        if (is_const_number(node->as.binary.left) && is_const_number(node->as.binary.right) &&
            node->as.binary.right->as.number != 0) {
            return make_number(pool,
                               node->as.binary.left->as.number % node->as.binary.right->as.number,
                               node->line, node->column);
        }
        break;
    case AST_BAND:
        if (is_const_number(node->as.binary.left) && is_const_number(node->as.binary.right)) {
            return make_number(pool,
                               node->as.binary.left->as.number & node->as.binary.right->as.number,
                               node->line, node->column);
        }
        // Identity: (& x 0xFF) -> x (for u8, mask is noop)
        if (is_const_number(node->as.binary.right) && node->as.binary.right->as.number == 0xFF)
            return node->as.binary.left;
        if (is_const_number(node->as.binary.left) && node->as.binary.left->as.number == 0xFF)
            return node->as.binary.right;
        // Annihilation: (& x 0) -> 0
        if (is_const_number(node->as.binary.right) && node->as.binary.right->as.number == 0)
            return make_number(pool, 0, node->line, node->column);
        if (is_const_number(node->as.binary.left) && node->as.binary.left->as.number == 0)
            return make_number(pool, 0, node->line, node->column);
        break;
    case AST_BOR:
        if (is_const_number(node->as.binary.left) && is_const_number(node->as.binary.right)) {
            return make_number(pool,
                               node->as.binary.left->as.number | node->as.binary.right->as.number,
                               node->line, node->column);
        }
        // Identity: (| x 0) -> x, (| 0 x) -> x
        if (is_const_number(node->as.binary.right) && node->as.binary.right->as.number == 0)
            return node->as.binary.left;
        if (is_const_number(node->as.binary.left) && node->as.binary.left->as.number == 0)
            return node->as.binary.right;
        break;
    case AST_XOR:
        if (is_const_number(node->as.binary.left) && is_const_number(node->as.binary.right)) {
            return make_number(pool,
                               node->as.binary.left->as.number ^ node->as.binary.right->as.number,
                               node->line, node->column);
        }
        // Identity: (^ x 0) -> x, (^ 0 x) -> x
        if (is_const_number(node->as.binary.right) && node->as.binary.right->as.number == 0)
            return node->as.binary.left;
        if (is_const_number(node->as.binary.left) && node->as.binary.left->as.number == 0)
            return node->as.binary.right;
        break;
    case AST_SHL:
        if (is_const_number(node->as.binary.left) && is_const_number(node->as.binary.right)) {
            return make_number(pool,
                               node->as.binary.left->as.number << node->as.binary.right->as.number,
                               node->line, node->column);
        }
        // Identity: (<< x 0) -> x
        if (is_const_number(node->as.binary.right) && node->as.binary.right->as.number == 0)
            return node->as.binary.left;
        break;
    case AST_SHR:
        if (is_const_number(node->as.binary.left) && is_const_number(node->as.binary.right)) {
            return make_number(pool,
                               node->as.binary.left->as.number >> node->as.binary.right->as.number,
                               node->line, node->column);
        }
        // Identity: (>> x 0) -> x
        if (is_const_number(node->as.binary.right) && node->as.binary.right->as.number == 0)
            return node->as.binary.left;
        break;
    case AST_EQ:
        if (is_const_number(node->as.binary.left) && is_const_number(node->as.binary.right)) {
            return make_number(
                pool, node->as.binary.left->as.number == node->as.binary.right->as.number ? 1 : 0,
                node->line, node->column);
        }
        break;
    case AST_NE:
        if (is_const_number(node->as.binary.left) && is_const_number(node->as.binary.right)) {
            return make_number(
                pool, node->as.binary.left->as.number != node->as.binary.right->as.number ? 1 : 0,
                node->line, node->column);
        }
        break;
    case AST_LT:
        if (is_const_number(node->as.binary.left) && is_const_number(node->as.binary.right)) {
            return make_number(
                pool, node->as.binary.left->as.number < node->as.binary.right->as.number ? 1 : 0,
                node->line, node->column);
        }
        break;
    case AST_GT:
        if (is_const_number(node->as.binary.left) && is_const_number(node->as.binary.right)) {
            return make_number(
                pool, node->as.binary.left->as.number > node->as.binary.right->as.number ? 1 : 0,
                node->line, node->column);
        }
        break;
    case AST_LE:
        if (is_const_number(node->as.binary.left) && is_const_number(node->as.binary.right)) {
            return make_number(
                pool, node->as.binary.left->as.number <= node->as.binary.right->as.number ? 1 : 0,
                node->line, node->column);
        }
        break;
    case AST_GE:
        if (is_const_number(node->as.binary.left) && is_const_number(node->as.binary.right)) {
            return make_number(
                pool, node->as.binary.left->as.number >= node->as.binary.right->as.number ? 1 : 0,
                node->line, node->column);
        }
        break;
    case AST_NEG:
        if (is_const_number(node->as.unary.operand)) {
            return make_number(pool, -node->as.unary.operand->as.number, node->line, node->column);
        }
        break;
    case AST_INC:
        if (is_const_number(node->as.unary.operand)) {
            return make_number(pool, node->as.unary.operand->as.number + 1, node->line,
                               node->column);
        }
        break;
    case AST_DEC:
        if (is_const_number(node->as.unary.operand)) {
            return make_number(pool, node->as.unary.operand->as.number - 1, node->line,
                               node->column);
        }
        break;
    case AST_BNOT:
        if (is_const_number(node->as.unary.operand)) {
            return make_number(pool, ~node->as.unary.operand->as.number & 0xFF, node->line,
                               node->column);
        }
        break;
    case AST_HI:
        if (is_const_number(node->as.unary.operand)) {
            return make_number(pool, (node->as.unary.operand->as.number >> 8) & 0xFF, node->line,
                               node->column);
        }
        break;
    case AST_LO:
        if (is_const_number(node->as.unary.operand)) {
            return make_number(pool, node->as.unary.operand->as.number & 0xFF, node->line,
                               node->column);
        }
        break;
    case AST_CAST_U8:
        if (is_const_number(node->as.unary.operand)) {
            return make_number(pool, node->as.unary.operand->as.number & 0xFF, node->line,
                               node->column);
        }
        // Redundant cast: (u8 (u8 x)) -> (u8 x)
        if (node->as.unary.operand->kind == AST_CAST_U8)
            return node->as.unary.operand;
        break;
    case AST_CAST_I8:
        if (is_const_number(node->as.unary.operand)) {
            int8_t v = (int8_t)(node->as.unary.operand->as.number & 0xFF);
            return make_number(pool, (int32_t)v, node->line, node->column);
        }
        // Redundant cast: (i8 (i8 x)) -> (i8 x)
        if (node->as.unary.operand->kind == AST_CAST_I8)
            return node->as.unary.operand;
        break;
    case AST_IF:
        // Fold constant conditions
        if (is_const_number(node->as.if_expr.cond)) {
            if (node->as.if_expr.cond->as.number != 0) {
                return node->as.if_expr.then_branch;
            } else {
                return node->as.if_expr.else_branch;
            }
        }
        if (node->as.if_expr.cond->kind == AST_TRUE) return node->as.if_expr.then_branch;
        if (node->as.if_expr.cond->kind == AST_FALSE) return node->as.if_expr.else_branch;
        break;
    case AST_WHEN:
        // (when true body...) -> (do body...)
        if (node->as.when_expr.cond->kind == AST_TRUE ||
            (is_const_number(node->as.when_expr.cond) &&
             node->as.when_expr.cond->as.number != 0)) {
            if (node->as.when_expr.body.count == 1)
                return node->as.when_expr.body.items[0];
            node->kind = AST_DO;
            node->as.block.exprs = node->as.when_expr.body;
            return node;
        }
        // (when false body...) -> nil
        if (node->as.when_expr.cond->kind == AST_FALSE ||
            (is_const_number(node->as.when_expr.cond) &&
             node->as.when_expr.cond->as.number == 0)) {
            AstNode* nil = ast_alloc(pool);
            if (nil) { nil->kind = AST_NIL; nil->line = node->line; nil->column = node->column; }
            return nil;
        }
        break;
    case AST_UNLESS:
        // (unless false body...) -> (do body...)
        if (node->as.when_expr.cond->kind == AST_FALSE ||
            (is_const_number(node->as.when_expr.cond) &&
             node->as.when_expr.cond->as.number == 0)) {
            if (node->as.when_expr.body.count == 1)
                return node->as.when_expr.body.items[0];
            node->kind = AST_DO;
            node->as.block.exprs = node->as.when_expr.body;
            return node;
        }
        // (unless true body...) -> nil
        if (node->as.when_expr.cond->kind == AST_TRUE ||
            (is_const_number(node->as.when_expr.cond) &&
             node->as.when_expr.cond->as.number != 0)) {
            AstNode* nil = ast_alloc(pool);
            if (nil) { nil->kind = AST_NIL; nil->line = node->line; nil->column = node->column; }
            return nil;
        }
        break;
    case AST_LOGIC_NOT:
        // (not true) -> false, (not false) -> true
        if (node->as.unary.operand->kind == AST_TRUE) {
            node->kind = AST_FALSE;
            return node;
        }
        if (node->as.unary.operand->kind == AST_FALSE) {
            node->kind = AST_TRUE;
            return node;
        }
        if (is_const_number(node->as.unary.operand)) {
            return make_number(pool, node->as.unary.operand->as.number == 0 ? 1 : 0,
                               node->line, node->column);
        }
        // (not (not x)) -> x (double negation elimination)
        if (node->as.unary.operand->kind == AST_LOGIC_NOT)
            return node->as.unary.operand->as.unary.operand;
        break;
    default: break;
    }

    return node;
}

static void pass_constant_fold(AstProgram* program, AstPool* pool) {
    for (size_t i = 0; i < program->node_count; i++) {
        program->nodes[i] = fold_node(pool, program->nodes[i]);
    }
}

// -------------------------------------------------------------------
// Strength reduction – convert multiply by power of 2 to shifts
// -------------------------------------------------------------------

static AstNode* strength_node(AstPool* pool, AstNode* node);

static void strength_array(AstPool* pool, AstNodeArray* arr) {
    for (size_t i = 0; i < arr->count; i++) {
        arr->items[i] = strength_node(pool, arr->items[i]);
    }
}

static int log2_exact(int32_t v) {
    if (v <= 0) return -1;
    int n = 0;
    while (v > 1) {
        if (v & 1) return -1; // not power of 2
        v >>= 1;
        n++;
    }
    return n;
}

static AstNode* strength_node(AstPool* pool, AstNode* node) {
    if (!node) return NULL;

    // Recurse first
    switch (node->kind) {
    case AST_DEF: node->as.def.value = strength_node(pool, node->as.def.value); break;
    case AST_DEFN:
    case AST_FN:
    case AST_DEFMACRO: strength_array(pool, &node->as.defn.body); break;
    case AST_LET:
        for (size_t i = 0; i < node->as.let.binding_count; i++)
            node->as.let.vals[i] = strength_node(pool, node->as.let.vals[i]);
        strength_array(pool, &node->as.let.body);
        break;
    case AST_VAR: node->as.var.value = strength_node(pool, node->as.var.value); break;
    case AST_IF:
        node->as.if_expr.cond = strength_node(pool, node->as.if_expr.cond);
        node->as.if_expr.then_branch = strength_node(pool, node->as.if_expr.then_branch);
        node->as.if_expr.else_branch = strength_node(pool, node->as.if_expr.else_branch);
        break;
    case AST_WHILE:
        node->as.while_expr.cond = strength_node(pool, node->as.while_expr.cond);
        strength_array(pool, &node->as.while_expr.body);
        break;
    case AST_DO: strength_array(pool, &node->as.block.exprs); break;
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
        node->as.binary.left = strength_node(pool, node->as.binary.left);
        node->as.binary.right = strength_node(pool, node->as.binary.right);
        break;
    case AST_NEG:
    case AST_INC:
    case AST_DEC:
    case AST_BNOT:
    case AST_LNOT:
    case AST_LOGIC_NOT:
    case AST_HI:
    case AST_LO:
    case AST_LEN:
    case AST_NILP:
    case AST_ZEROP:
    case AST_POSP:
    case AST_NEGP:
    case AST_CAST_U8:
    case AST_CAST_I8: node->as.unary.operand = strength_node(pool, node->as.unary.operand); break;
    case AST_CALL:
        for (size_t i = 0; i < node->as.call.arg_count; i++)
            node->as.call.args[i] = strength_node(pool, node->as.call.args[i]);
        break;
    case AST_COND:
        for (size_t i = 0; i < node->as.cond.clause_count; i++) {
            node->as.cond.tests[i] = strength_node(pool, node->as.cond.tests[i]);
            strength_array(pool, &node->as.cond.bodies[i]);
        }
        break;
    case AST_WHEN:
    case AST_UNLESS:
        node->as.when_expr.cond = strength_node(pool, node->as.when_expr.cond);
        strength_array(pool, &node->as.when_expr.body);
        break;
    case AST_FOR:
        node->as.for_expr.collection = strength_node(pool, node->as.for_expr.collection);
        if (node->as.for_expr.when_cond)
            node->as.for_expr.when_cond = strength_node(pool, node->as.for_expr.when_cond);
        strength_array(pool, &node->as.for_expr.body);
        break;
    case AST_SET:
    case AST_SET_BANG:
        node->as.set.value = strength_node(pool, node->as.set.value);
        if (node->as.set.target_expr)
            node->as.set.target_expr = strength_node(pool, node->as.set.target_expr);
        break;
    default: break;
    }

    // Strength reduce: (* x 2^n) -> (<< x n)
    if (node->kind == AST_MUL) {
        if (is_const_number(node->as.binary.right)) {
            int shift = log2_exact(node->as.binary.right->as.number);
            if (shift > 0) {
                AstNode* shift_node = make_number(pool, shift, node->line, node->column);
                if (shift_node) {
                    node->kind = AST_SHL;
                    node->as.binary.right = shift_node;
                }
            }
        } else if (is_const_number(node->as.binary.left)) {
            int shift = log2_exact(node->as.binary.left->as.number);
            if (shift > 0) {
                AstNode* shift_node = make_number(pool, shift, node->line, node->column);
                if (shift_node) {
                    node->kind = AST_SHL;
                    node->as.binary.left = node->as.binary.right;
                    node->as.binary.right = shift_node;
                }
            }
        }
    }

    // Strength reduce: (/ x 2^n) -> (>> x n)
    if (node->kind == AST_DIV && is_const_number(node->as.binary.right)) {
        int shift = log2_exact(node->as.binary.right->as.number);
        if (shift > 0) {
            AstNode* shift_node = make_number(pool, shift, node->line, node->column);
            if (shift_node) {
                node->kind = AST_SHR;
                node->as.binary.right = shift_node;
            }
        }
    }

    // Strength reduce: (% x 2^n) -> (& x (2^n - 1))
    if (node->kind == AST_MOD && is_const_number(node->as.binary.right)) {
        int shift = log2_exact(node->as.binary.right->as.number);
        if (shift > 0) {
            int32_t mask = node->as.binary.right->as.number - 1;
            AstNode* mask_node = make_number(pool, mask, node->line, node->column);
            if (mask_node) {
                node->kind = AST_BAND;
                node->as.binary.right = mask_node;
            }
        }
    }

    // Strength reduce: (+ x x) -> (<< x 1)
    if (node->kind == AST_ADD && node->as.binary.left && node->as.binary.right &&
        node->as.binary.left->kind == AST_SYMBOL && node->as.binary.right->kind == AST_SYMBOL &&
        strcmp(node->as.binary.left->as.symbol.name, node->as.binary.right->as.symbol.name) == 0) {
        AstNode* one = make_number(pool, 1, node->line, node->column);
        if (one) {
            node->kind = AST_SHL;
            node->as.binary.right = one;
        }
    }

    return node;
}

static void pass_strength_reduce(AstProgram* program, AstPool* pool) {
    for (size_t i = 0; i < program->node_count; i++) {
        program->nodes[i] = strength_node(pool, program->nodes[i]);
    }
}

// -------------------------------------------------------------------
// Dead code elimination – remove defmacro nodes from the program
// (macros have already been expanded and are no longer needed)
// Also remove unreachable code after constant-folded conditions.
// -------------------------------------------------------------------

// Check if a symbol name is referenced anywhere in a subtree
static bool name_used_in(const char* name, AstNode* node) {
    if (!node) return false;

    if (node->kind == AST_SYMBOL && strcmp(node->as.symbol.name, name) == 0) return true;

    switch (node->kind) {
    case AST_DEF: return name_used_in(name, node->as.def.value);
    case AST_DEFN:
    case AST_FN:
    case AST_DEFMACRO:
        for (size_t i = 0; i < node->as.defn.body.count; i++)
            if (name_used_in(name, node->as.defn.body.items[i])) return true;
        return false;
    case AST_LET:
        for (size_t i = 0; i < node->as.let.binding_count; i++)
            if (name_used_in(name, node->as.let.vals[i])) return true;
        for (size_t i = 0; i < node->as.let.body.count; i++)
            if (name_used_in(name, node->as.let.body.items[i])) return true;
        return false;
    case AST_VAR: return name_used_in(name, node->as.var.value);
    case AST_SET:
    case AST_SET_BANG:
        if (strcmp(node->as.set.var, name) == 0) return true;
        if (name_used_in(name, node->as.set.value)) return true;
        return name_used_in(name, node->as.set.target_expr);
    case AST_IF:
        return name_used_in(name, node->as.if_expr.cond) ||
               name_used_in(name, node->as.if_expr.then_branch) ||
               name_used_in(name, node->as.if_expr.else_branch);
    case AST_WHILE:
        if (name_used_in(name, node->as.while_expr.cond)) return true;
        for (size_t i = 0; i < node->as.while_expr.body.count; i++)
            if (name_used_in(name, node->as.while_expr.body.items[i])) return true;
        return false;
    case AST_COND:
        for (size_t i = 0; i < node->as.cond.clause_count; i++) {
            if (name_used_in(name, node->as.cond.tests[i])) return true;
            for (size_t j = 0; j < node->as.cond.bodies[i].count; j++)
                if (name_used_in(name, node->as.cond.bodies[i].items[j])) return true;
        }
        return false;
    case AST_WHEN:
    case AST_UNLESS:
        if (name_used_in(name, node->as.when_expr.cond)) return true;
        for (size_t i = 0; i < node->as.when_expr.body.count; i++)
            if (name_used_in(name, node->as.when_expr.body.items[i])) return true;
        return false;
    case AST_FOR:
        if (name_used_in(name, node->as.for_expr.collection)) return true;
        if (name_used_in(name, node->as.for_expr.when_cond)) return true;
        for (size_t i = 0; i < node->as.for_expr.body.count; i++)
            if (name_used_in(name, node->as.for_expr.body.items[i])) return true;
        return false;
    case AST_RANGE:
        return name_used_in(name, node->as.range.start) || name_used_in(name, node->as.range.end);
    case AST_DO:
    case AST_DB:
    case AST_REQUIRE:
        for (size_t i = 0; i < node->as.block.exprs.count; i++)
            if (name_used_in(name, node->as.block.exprs.items[i])) return true;
        return false;
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
        return name_used_in(name, node->as.binary.left) ||
               name_used_in(name, node->as.binary.right);
    case AST_NEG:
    case AST_INC:
    case AST_DEC:
    case AST_BNOT:
    case AST_LNOT:
    case AST_LOGIC_NOT:
    case AST_HI:
    case AST_LO:
    case AST_LEN:
    case AST_NILP:
    case AST_ZEROP:
    case AST_POSP:
    case AST_NEGP:
    case AST_CAST_U8:
    case AST_CAST_I8: return name_used_in(name, node->as.unary.operand);
    case AST_CALL:
        if (strcmp(node->as.call.func, name) == 0) return true;
        for (size_t i = 0; i < node->as.call.arg_count; i++)
            if (name_used_in(name, node->as.call.args[i])) return true;
        return false;
    case AST_LOAD: return name_used_in(name, node->as.load.addr);
    case AST_STORE:
        return name_used_in(name, node->as.store.addr) || name_used_in(name, node->as.store.value);
    case AST_FIELD_GET: return name_used_in(name, node->as.field_get.record);
    case AST_ARRAY:
        return name_used_in(name, node->as.array_expr.count) ||
               name_used_in(name, node->as.array_expr.value);
    default: return false;
    }
}

// Remove unused let bindings from a node (recursively)
static AstNode* dce_node(AstNode* node) {
    if (!node) return NULL;

    // Recurse into children first
    switch (node->kind) {
    case AST_DEF: node->as.def.value = dce_node(node->as.def.value); break;
    case AST_DEFN:
    case AST_FN:
    case AST_DEFMACRO:
        for (size_t i = 0; i < node->as.defn.body.count; i++)
            node->as.defn.body.items[i] = dce_node(node->as.defn.body.items[i]);
        break;
    case AST_VAR: node->as.var.value = dce_node(node->as.var.value); break;
    case AST_IF:
        node->as.if_expr.cond = dce_node(node->as.if_expr.cond);
        node->as.if_expr.then_branch = dce_node(node->as.if_expr.then_branch);
        node->as.if_expr.else_branch = dce_node(node->as.if_expr.else_branch);
        break;
    case AST_WHILE:
        node->as.while_expr.cond = dce_node(node->as.while_expr.cond);
        for (size_t i = 0; i < node->as.while_expr.body.count; i++)
            node->as.while_expr.body.items[i] = dce_node(node->as.while_expr.body.items[i]);
        break;
    case AST_DO:
        for (size_t i = 0; i < node->as.block.exprs.count; i++)
            node->as.block.exprs.items[i] = dce_node(node->as.block.exprs.items[i]);
        break;
    case AST_COND:
        for (size_t i = 0; i < node->as.cond.clause_count; i++) {
            node->as.cond.tests[i] = dce_node(node->as.cond.tests[i]);
            for (size_t j = 0; j < node->as.cond.bodies[i].count; j++)
                node->as.cond.bodies[i].items[j] = dce_node(node->as.cond.bodies[i].items[j]);
        }
        break;
    case AST_WHEN:
    case AST_UNLESS:
        node->as.when_expr.cond = dce_node(node->as.when_expr.cond);
        for (size_t i = 0; i < node->as.when_expr.body.count; i++)
            node->as.when_expr.body.items[i] = dce_node(node->as.when_expr.body.items[i]);
        break;
    case AST_FOR:
        node->as.for_expr.collection = dce_node(node->as.for_expr.collection);
        if (node->as.for_expr.when_cond)
            node->as.for_expr.when_cond = dce_node(node->as.for_expr.when_cond);
        for (size_t i = 0; i < node->as.for_expr.body.count; i++)
            node->as.for_expr.body.items[i] = dce_node(node->as.for_expr.body.items[i]);
        break;
    default: break;
    }

    // Now check let bindings for unused variables
    if (node->kind == AST_LET) {
        // First recurse into binding values and body
        for (size_t i = 0; i < node->as.let.binding_count; i++)
            node->as.let.vals[i] = dce_node(node->as.let.vals[i]);
        for (size_t i = 0; i < node->as.let.body.count; i++)
            node->as.let.body.items[i] = dce_node(node->as.let.body.items[i]);

        // Check each binding: is the variable used in subsequent bindings or body?
        size_t write = 0;
        for (size_t i = 0; i < node->as.let.binding_count; i++) {
            bool used = false;
            // Check in subsequent binding values
            for (size_t j = i + 1; j < node->as.let.binding_count; j++) {
                if (name_used_in(node->as.let.vars[i], node->as.let.vals[j])) {
                    used = true;
                    break;
                }
            }
            // Check in body
            if (!used) {
                for (size_t j = 0; j < node->as.let.body.count; j++) {
                    if (name_used_in(node->as.let.vars[i], node->as.let.body.items[j])) {
                        used = true;
                        break;
                    }
                }
            }
            if (used) {
                // Keep this binding (compact if needed)
                if (write != i) {
                    memcpy(node->as.let.vars[write], node->as.let.vars[i], SE_MAX_SYMBOL_LEN);
                    node->as.let.vals[write] = node->as.let.vals[i];
                    node->as.let.hints[write] = node->as.let.hints[i];
                }
                write++;
            }
            // If not used, skip (dead binding)
        }
        node->as.let.binding_count = write;

        // If no bindings remain, unwrap: return the body as a do block or single expr
        if (write == 0) {
            if (node->as.let.body.count == 1) {
                return node->as.let.body.items[0];
            }
            // Convert to do block
            node->kind = AST_DO;
            node->as.block.exprs = node->as.let.body;
        }
    }

    return node;
}

static void pass_dead_code(AstProgram* program) {
    // Remove defmacro nodes (they've been expanded already)
    size_t write = 0;
    for (size_t i = 0; i < program->node_count; i++) {
        if (program->nodes[i]->kind != AST_DEFMACRO) {
            program->nodes[write++] = program->nodes[i];
        }
    }
    program->node_count = write;

    // Remove unused let bindings
    for (size_t i = 0; i < program->node_count; i++) {
        program->nodes[i] = dce_node(program->nodes[i]);
    }
}

// -------------------------------------------------------------------
// Constant propagation – propagate def constants into expressions
// -------------------------------------------------------------------

typedef struct {
    char name[SE_MAX_SYMBOL_LEN];
    int32_t value;
} ConstBinding;

#define MAX_CONST_BINDINGS 256

typedef struct {
    ConstBinding bindings[MAX_CONST_BINDINGS];
    size_t count;
    AstPool* pool;
} ConstPropCtx;

static void cprop_add(ConstPropCtx* ctx, const char* name, int32_t value) {
    if (ctx->count >= MAX_CONST_BINDINGS) return;
    strncpy(ctx->bindings[ctx->count].name, name, SE_MAX_SYMBOL_LEN - 1);
    ctx->bindings[ctx->count].name[SE_MAX_SYMBOL_LEN - 1] = '\0';
    ctx->bindings[ctx->count].value = value;
    ctx->count++;
}

static bool cprop_lookup(ConstPropCtx* ctx, const char* name, int32_t* value) {
    for (size_t i = 0; i < ctx->count; i++) {
        if (strcmp(ctx->bindings[i].name, name) == 0) {
            *value = ctx->bindings[i].value;
            return true;
        }
    }
    return false;
}

static AstNode* cprop_node(ConstPropCtx* ctx, AstNode* node);

static void cprop_array(ConstPropCtx* ctx, AstNodeArray* arr) {
    for (size_t i = 0; i < arr->count; i++) {
        arr->items[i] = cprop_node(ctx, arr->items[i]);
    }
}

// Check if a variable name is the target of any (set! name ...) in a subtree.
// Used to prevent constant propagation of mutable let bindings.
static bool is_set_target_in(const char* name, AstNode* node) {
    if (!node) return false;

    if ((node->kind == AST_SET || node->kind == AST_SET_BANG) &&
        strcmp(node->as.set.var, name) == 0) {
        return true;
    }

    switch (node->kind) {
    case AST_LET:
        // If the let shadows the variable name, don't look further into its body
        for (size_t i = 0; i < node->as.let.binding_count; i++) {
            if (strcmp(node->as.let.vars[i], name) == 0) {
                // Check only values up to the shadowing point
                for (size_t j = 0; j < i; j++)
                    if (is_set_target_in(name, node->as.let.vals[j])) return true;
                return false;
            }
            if (is_set_target_in(name, node->as.let.vals[i])) return true;
        }
        for (size_t i = 0; i < node->as.let.body.count; i++)
            if (is_set_target_in(name, node->as.let.body.items[i])) return true;
        return false;
    case AST_DEFN:
    case AST_FN:
    case AST_DEFMACRO:
        for (size_t i = 0; i < node->as.defn.body.count; i++)
            if (is_set_target_in(name, node->as.defn.body.items[i])) return true;
        return false;
    case AST_IF:
        return is_set_target_in(name, node->as.if_expr.cond) ||
               is_set_target_in(name, node->as.if_expr.then_branch) ||
               is_set_target_in(name, node->as.if_expr.else_branch);
    case AST_WHILE:
        if (is_set_target_in(name, node->as.while_expr.cond)) return true;
        for (size_t i = 0; i < node->as.while_expr.body.count; i++)
            if (is_set_target_in(name, node->as.while_expr.body.items[i])) return true;
        return false;
    case AST_DO:
    case AST_DB:
    case AST_REQUIRE:
        for (size_t i = 0; i < node->as.block.exprs.count; i++)
            if (is_set_target_in(name, node->as.block.exprs.items[i])) return true;
        return false;
    case AST_COND:
        for (size_t i = 0; i < node->as.cond.clause_count; i++) {
            if (is_set_target_in(name, node->as.cond.tests[i])) return true;
            for (size_t j = 0; j < node->as.cond.bodies[i].count; j++)
                if (is_set_target_in(name, node->as.cond.bodies[i].items[j])) return true;
        }
        return false;
    case AST_WHEN:
    case AST_UNLESS:
        if (is_set_target_in(name, node->as.when_expr.cond)) return true;
        for (size_t i = 0; i < node->as.when_expr.body.count; i++)
            if (is_set_target_in(name, node->as.when_expr.body.items[i])) return true;
        return false;
    case AST_FOR:
        if (is_set_target_in(name, node->as.for_expr.collection)) return true;
        if (is_set_target_in(name, node->as.for_expr.when_cond)) return true;
        for (size_t i = 0; i < node->as.for_expr.body.count; i++)
            if (is_set_target_in(name, node->as.for_expr.body.items[i])) return true;
        return false;
    case AST_SET:
    case AST_SET_BANG:
        return is_set_target_in(name, node->as.set.value) ||
               is_set_target_in(name, node->as.set.target_expr);
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
        return is_set_target_in(name, node->as.binary.left) ||
               is_set_target_in(name, node->as.binary.right);
    case AST_NEG:
    case AST_INC:
    case AST_DEC:
    case AST_BNOT:
    case AST_LNOT:
    case AST_LOGIC_NOT:
    case AST_HI:
    case AST_LO:
    case AST_LEN:
    case AST_NILP:
    case AST_ZEROP:
    case AST_POSP:
    case AST_NEGP:
    case AST_CAST_U8:
    case AST_CAST_I8: return is_set_target_in(name, node->as.unary.operand);
    case AST_CALL:
        for (size_t i = 0; i < node->as.call.arg_count; i++)
            if (is_set_target_in(name, node->as.call.args[i])) return true;
        return false;
    case AST_LOAD: return is_set_target_in(name, node->as.load.addr);
    case AST_STORE:
        return is_set_target_in(name, node->as.store.addr) ||
               is_set_target_in(name, node->as.store.value);
    case AST_FIELD_GET: return is_set_target_in(name, node->as.field_get.record);
    case AST_RANGE:
        return is_set_target_in(name, node->as.range.start) ||
               is_set_target_in(name, node->as.range.end);
    default: return false;
    }
}

static AstNode* cprop_node(ConstPropCtx* ctx, AstNode* node) {
    if (!node) return NULL;

    // Replace symbols with known constant values
    if (node->kind == AST_SYMBOL) {
        int32_t value;
        if (cprop_lookup(ctx, node->as.symbol.name, &value)) {
            return make_number(ctx->pool, value, node->line, node->column);
        }
        return node;
    }

    // Collect constants from let bindings
    if (node->kind == AST_LET) {
        size_t saved_count = ctx->count;
        for (size_t i = 0; i < node->as.let.binding_count; i++) {
            node->as.let.vals[i] = cprop_node(ctx, node->as.let.vals[i]);
            if (is_const_number(node->as.let.vals[i])) {
                // Only propagate if the binding is never mutated via set!
                // in subsequent bindings or body
                bool mutated = false;
                for (size_t j = i + 1; j < node->as.let.binding_count; j++) {
                    if (is_set_target_in(node->as.let.vars[i], node->as.let.vals[j])) {
                        mutated = true;
                        break;
                    }
                }
                if (!mutated) {
                    for (size_t j = 0; j < node->as.let.body.count; j++) {
                        if (is_set_target_in(node->as.let.vars[i], node->as.let.body.items[j])) {
                            mutated = true;
                            break;
                        }
                    }
                }
                if (!mutated) {
                    cprop_add(ctx, node->as.let.vars[i], node->as.let.vals[i]->as.number);
                }
            }
        }
        cprop_array(ctx, &node->as.let.body);
        ctx->count = saved_count;
        return node;
    }

    // Recurse into children
    switch (node->kind) {
    case AST_DEF: node->as.def.value = cprop_node(ctx, node->as.def.value); break;
    case AST_DEFN:
    case AST_FN:
    case AST_DEFMACRO: cprop_array(ctx, &node->as.defn.body); break;
    case AST_VAR: node->as.var.value = cprop_node(ctx, node->as.var.value); break;
    case AST_SET:
    case AST_SET_BANG:
        node->as.set.value = cprop_node(ctx, node->as.set.value);
        if (node->as.set.target_expr)
            node->as.set.target_expr = cprop_node(ctx, node->as.set.target_expr);
        break;
    case AST_IF:
        node->as.if_expr.cond = cprop_node(ctx, node->as.if_expr.cond);
        node->as.if_expr.then_branch = cprop_node(ctx, node->as.if_expr.then_branch);
        node->as.if_expr.else_branch = cprop_node(ctx, node->as.if_expr.else_branch);
        break;
    case AST_WHILE:
        node->as.while_expr.cond = cprop_node(ctx, node->as.while_expr.cond);
        cprop_array(ctx, &node->as.while_expr.body);
        break;
    case AST_DO:
    case AST_DB:
    case AST_REQUIRE: cprop_array(ctx, &node->as.block.exprs); break;
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
        node->as.binary.left = cprop_node(ctx, node->as.binary.left);
        node->as.binary.right = cprop_node(ctx, node->as.binary.right);
        break;
    case AST_NEG:
    case AST_INC:
    case AST_DEC:
    case AST_BNOT:
    case AST_LNOT:
    case AST_LOGIC_NOT:
    case AST_HI:
    case AST_LO:
    case AST_LEN:
    case AST_NILP:
    case AST_ZEROP:
    case AST_POSP:
    case AST_NEGP:
    case AST_CAST_U8:
    case AST_CAST_I8: node->as.unary.operand = cprop_node(ctx, node->as.unary.operand); break;
    case AST_CALL:
        for (size_t i = 0; i < node->as.call.arg_count; i++)
            node->as.call.args[i] = cprop_node(ctx, node->as.call.args[i]);
        break;
    case AST_COND:
        for (size_t i = 0; i < node->as.cond.clause_count; i++) {
            node->as.cond.tests[i] = cprop_node(ctx, node->as.cond.tests[i]);
            cprop_array(ctx, &node->as.cond.bodies[i]);
        }
        break;
    case AST_WHEN:
    case AST_UNLESS:
        node->as.when_expr.cond = cprop_node(ctx, node->as.when_expr.cond);
        cprop_array(ctx, &node->as.when_expr.body);
        break;
    case AST_FOR:
        node->as.for_expr.collection = cprop_node(ctx, node->as.for_expr.collection);
        if (node->as.for_expr.when_cond)
            node->as.for_expr.when_cond = cprop_node(ctx, node->as.for_expr.when_cond);
        cprop_array(ctx, &node->as.for_expr.body);
        break;
    case AST_RANGE:
        node->as.range.start = cprop_node(ctx, node->as.range.start);
        node->as.range.end = cprop_node(ctx, node->as.range.end);
        break;
    case AST_LOAD: node->as.load.addr = cprop_node(ctx, node->as.load.addr); break;
    case AST_STORE:
        node->as.store.addr = cprop_node(ctx, node->as.store.addr);
        node->as.store.value = cprop_node(ctx, node->as.store.value);
        break;
    case AST_FIELD_GET:
        node->as.field_get.record = cprop_node(ctx, node->as.field_get.record);
        break;
    case AST_ARRAY:
        node->as.array_expr.count = cprop_node(ctx, node->as.array_expr.count);
        node->as.array_expr.value = cprop_node(ctx, node->as.array_expr.value);
        break;
    default: break;
    }

    return node;
}

static void pass_constant_prop(AstProgram* program, AstPool* pool) {
    ConstPropCtx ctx;
    ctx.count = 0;
    ctx.pool = pool;

    // Seed with top-level def constants
    for (size_t i = 0; i < program->node_count; i++) {
        AstNode* node = program->nodes[i];
        if (node->kind == AST_DEF && is_const_number(node->as.def.value)) {
            cprop_add(&ctx, node->as.def.name, node->as.def.value->as.number);
        }
    }

    for (size_t i = 0; i < program->node_count; i++) {
        program->nodes[i] = cprop_node(&ctx, program->nodes[i]);
    }
}

// -------------------------------------------------------------------
// Dead function elimination – remove defn nodes for uncalled functions
// -------------------------------------------------------------------

#define MAX_FUNC_NAMES 512

typedef struct {
    char names[MAX_FUNC_NAMES][SE_MAX_SYMBOL_LEN];
    size_t count;
} FuncSet;

static bool fset_contains(FuncSet* set, const char* name) {
    for (size_t i = 0; i < set->count; i++) {
        if (strcmp(set->names[i], name) == 0) return true;
    }
    return false;
}

static void fset_add(FuncSet* set, const char* name) {
    if (set->count >= MAX_FUNC_NAMES || fset_contains(set, name)) return;
    strncpy(set->names[set->count], name, SE_MAX_SYMBOL_LEN - 1);
    set->names[set->count][SE_MAX_SYMBOL_LEN - 1] = '\0';
    set->count++;
}

// Collect all function/call references in a subtree
static void collect_calls(AstNode* node, FuncSet* calls) {
    if (!node) return;

    if (node->kind == AST_CALL) {
        fset_add(calls, node->as.call.func);
        for (size_t i = 0; i < node->as.call.arg_count; i++)
            collect_calls(node->as.call.args[i], calls);
        return;
    }

    // A symbol might be a function reference (passed as value)
    // We can't easily distinguish, so we're conservative only for calls

    switch (node->kind) {
    case AST_DEFN:
    case AST_FN:
    case AST_DEFMACRO:
        for (size_t i = 0; i < node->as.defn.body.count; i++)
            collect_calls(node->as.defn.body.items[i], calls);
        break;
    case AST_LET:
        for (size_t i = 0; i < node->as.let.binding_count; i++)
            collect_calls(node->as.let.vals[i], calls);
        for (size_t i = 0; i < node->as.let.body.count; i++)
            collect_calls(node->as.let.body.items[i], calls);
        break;
    case AST_IF:
        collect_calls(node->as.if_expr.cond, calls);
        collect_calls(node->as.if_expr.then_branch, calls);
        collect_calls(node->as.if_expr.else_branch, calls);
        break;
    case AST_WHILE:
        collect_calls(node->as.while_expr.cond, calls);
        for (size_t i = 0; i < node->as.while_expr.body.count; i++)
            collect_calls(node->as.while_expr.body.items[i], calls);
        break;
    case AST_DO:
    case AST_DB:
    case AST_REQUIRE:
        for (size_t i = 0; i < node->as.block.exprs.count; i++)
            collect_calls(node->as.block.exprs.items[i], calls);
        break;
    case AST_COND:
        for (size_t i = 0; i < node->as.cond.clause_count; i++) {
            collect_calls(node->as.cond.tests[i], calls);
            for (size_t j = 0; j < node->as.cond.bodies[i].count; j++)
                collect_calls(node->as.cond.bodies[i].items[j], calls);
        }
        break;
    case AST_WHEN:
    case AST_UNLESS:
        collect_calls(node->as.when_expr.cond, calls);
        for (size_t i = 0; i < node->as.when_expr.body.count; i++)
            collect_calls(node->as.when_expr.body.items[i], calls);
        break;
    case AST_FOR:
        collect_calls(node->as.for_expr.collection, calls);
        collect_calls(node->as.for_expr.when_cond, calls);
        for (size_t i = 0; i < node->as.for_expr.body.count; i++)
            collect_calls(node->as.for_expr.body.items[i], calls);
        break;
    case AST_SET:
    case AST_SET_BANG:
        collect_calls(node->as.set.value, calls);
        collect_calls(node->as.set.target_expr, calls);
        break;
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
        collect_calls(node->as.binary.left, calls);
        collect_calls(node->as.binary.right, calls);
        break;
    case AST_NEG:
    case AST_INC:
    case AST_DEC:
    case AST_BNOT:
    case AST_LNOT:
    case AST_LOGIC_NOT:
    case AST_HI:
    case AST_LO:
    case AST_LEN:
    case AST_NILP:
    case AST_ZEROP:
    case AST_POSP:
    case AST_NEGP:
    case AST_CAST_U8:
    case AST_CAST_I8: collect_calls(node->as.unary.operand, calls); break;
    case AST_LOAD: collect_calls(node->as.load.addr, calls); break;
    case AST_STORE:
        collect_calls(node->as.store.addr, calls);
        collect_calls(node->as.store.value, calls);
        break;
    case AST_FIELD_GET: collect_calls(node->as.field_get.record, calls); break;
    case AST_RANGE:
        collect_calls(node->as.range.start, calls);
        collect_calls(node->as.range.end, calls);
        break;
    case AST_VAR: collect_calls(node->as.var.value, calls); break;
    default: break;
    }
}

static void pass_dead_fn_elim(AstProgram* program) {
    // Only run if "main" exists in the program
    bool has_main = false;
    for (size_t i = 0; i < program->node_count; i++) {
        if (program->nodes[i]->kind == AST_DEFN &&
            strcmp(program->nodes[i]->as.defn.name, "main") == 0) {
            has_main = true;
            break;
        }
    }
    if (!has_main) return;

    // Collect reachable functions starting from "main"
    FuncSet reachable;
    reachable.count = 0;

    // Seed with "main"
    fset_add(&reachable, "main");

    // Iteratively find all transitively called functions
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < program->node_count; i++) {
            AstNode* node = program->nodes[i];
            if (node->kind != AST_DEFN) continue;
            if (!fset_contains(&reachable, node->as.defn.name)) continue;

            // Collect all calls in this function
            FuncSet calls;
            calls.count = 0;
            for (size_t j = 0; j < node->as.defn.body.count; j++) {
                collect_calls(node->as.defn.body.items[j], &calls);
            }

            // Add newly discovered functions
            for (size_t j = 0; j < calls.count; j++) {
                if (!fset_contains(&reachable, calls.names[j])) {
                    fset_add(&reachable, calls.names[j]);
                    changed = true;
                }
            }
        }
    }

    // Remove unreachable defn nodes
    size_t write = 0;
    for (size_t i = 0; i < program->node_count; i++) {
        AstNode* node = program->nodes[i];
        if (node->kind == AST_DEFN && !fset_contains(&reachable, node->as.defn.name)) {
            continue; // Skip unreachable function
        }
        program->nodes[write++] = program->nodes[i];
    }
    program->node_count = write;
}

// -------------------------------------------------------------------
// Main optimizer entry point
// -------------------------------------------------------------------

void se_optimize(AstProgram* program, AstPool* pool, SeOptLevel level) {
    if (level == SE_OPT_NONE) return;

    // -O1 and -O2: basic passes
    pass_constant_prop(program, pool);
    pass_constant_fold(program, pool);
    pass_dead_code(program);

    if (level >= SE_OPT_FULL) {
        // -O2: additional passes
        pass_strength_reduce(program, pool);
        // Run constant fold again after strength reduction may expose new constants
        pass_constant_fold(program, pool);
        // Dead function elimination: remove functions never called from main
        pass_dead_fn_elim(program);
    }
}
