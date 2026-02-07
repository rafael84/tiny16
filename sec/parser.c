#include "parser.h"

#include <stdio.h>
#include <string.h>

const char* se_parse_error_name(SeParseError error) {
    switch (error) {
    case SE_PARSE_OK: return "OK";
    case SE_PARSE_ERROR_UNEXPECTED_TOKEN: return "unexpected token";
    case SE_PARSE_ERROR_EXPECTED_LPAREN: return "expected '('";
    case SE_PARSE_ERROR_EXPECTED_RPAREN: return "expected ')'";
    case SE_PARSE_ERROR_EXPECTED_SYMBOL: return "expected symbol";
    case SE_PARSE_ERROR_EXPECTED_NUMBER: return "expected number";
    case SE_PARSE_ERROR_UNKNOWN_FORM: return "unknown form";
    case SE_PARSE_ERROR_TOO_MANY_ARGS: return "too many arguments";
    case SE_PARSE_ERROR_TOO_FEW_ARGS: return "too few arguments";
    case SE_PARSE_ERROR_OUT_OF_MEMORY: return "out of memory";
    default: return "unknown error";
    }
}

static void advance(SeParser* parser) { parser->current = se_lexer_next(&parser->lexer); }

static void parser_set_error(SeParser* parser, SeParseError error, const char* msg) {
    if (parser->error != SE_PARSE_OK) return;
    parser->error = error;
    parser->error_line = parser->current.line;
    parser->error_column = parser->current.column;
    if (msg) {
        strncpy(parser->error_msg, msg, sizeof(parser->error_msg) - 1);
        parser->error_msg[sizeof(parser->error_msg) - 1] = '\0';
    } else {
        parser->error_msg[0] = '\0';
    }
}

static void copy_token_text(char* dest, SeToken* token, size_t max_len) {
    size_t len = token->text_len;
    if (len >= max_len) len = max_len - 1;
    memcpy(dest, token->text, len);
    dest[len] = '\0';
}

static AstNode* alloc_node(SeParser* parser, AstKind kind) {
    AstNode* node = ast_alloc(parser->pool);
    if (!node) {
        parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY, "AST pool exhausted");
        return NULL;
    }
    node->kind = kind;
    node->line = parser->current.line;
    node->column = parser->current.column;
    return node;
}

void se_parser_init(SeParser* parser, const char* source, size_t source_len, AstPool* pool) {
    parser->lexer = se_lexer_new(source, source_len);
    parser->pool = pool;
    parser->error = SE_PARSE_OK;
    parser->error_line = 0;
    parser->error_column = 0;
    parser->error_msg[0] = '\0';
    parser->fn_counter = 0;
    advance(parser);
}

static AstNode* parse_list(SeParser* parser);
static AstNode* parse_unary(SeParser* parser, AstKind kind);

AstNode* se_parser_parse_form(SeParser* parser) {
    if (parser->error != SE_PARSE_OK) return NULL;

    switch (parser->current.kind) {
    case SE_TOKEN_NUMBER: {
        AstNode* node = alloc_node(parser, AST_NUMBER);
        if (!node) return NULL;
        node->as.number = parser->current.number_value;
        advance(parser);
        return node;
    }

    case SE_TOKEN_STRING: {
        AstNode* node = alloc_node(parser, AST_STRING);
        if (!node) return NULL;
        // Skip quotes when copying
        size_t len = parser->current.text_len;
        if (len >= 2) {
            len -= 2; // remove quotes
            if (len >= SE_MAX_SYMBOL_LEN) len = SE_MAX_SYMBOL_LEN - 1;
            memcpy(node->as.symbol.name, parser->current.text + 1, len);
            node->as.symbol.name[len] = '\0';
        }
        advance(parser);
        return node;
    }

    case SE_TOKEN_SYMBOL: {
        AstNode* node = alloc_node(parser, AST_SYMBOL);
        if (!node) return NULL;
        copy_token_text(node->as.symbol.name, &parser->current, SE_MAX_SYMBOL_LEN);
        advance(parser);
        return node;
    }

    case SE_TOKEN_KEYWORD: {
        AstNode* node = alloc_node(parser, AST_KEYWORD);
        if (!node) return NULL;
        copy_token_text(node->as.symbol.name, &parser->current, SE_MAX_SYMBOL_LEN);
        advance(parser);
        return node;
    }

    case SE_TOKEN_NIL: {
        AstNode* node = alloc_node(parser, AST_NIL);
        advance(parser);
        return node;
    }

    case SE_TOKEN_TRUE: {
        AstNode* node = alloc_node(parser, AST_TRUE);
        advance(parser);
        return node;
    }

    case SE_TOKEN_FALSE: {
        AstNode* node = alloc_node(parser, AST_FALSE);
        advance(parser);
        return node;
    }

    case SE_TOKEN_LPAREN: return parse_list(parser);

    case SE_TOKEN_RPAREN:
        parser_set_error(parser, SE_PARSE_ERROR_UNEXPECTED_TOKEN, "unexpected ')'");
        return NULL;

    case SE_TOKEN_END: return NULL;

    default: parser_set_error(parser, SE_PARSE_ERROR_UNEXPECTED_TOKEN, NULL); return NULL;
    }
}

static bool is_symbol(SeParser* parser, const char* name) {
    return se_token_is_symbol(&parser->current, name);
}

// Try to parse a ^hint annotation. Returns SE_HINT_NONE if no hint present.
static SeTypeHint try_parse_hint(SeParser* parser) {
    if (parser->current.kind != SE_TOKEN_SYMBOL || parser->current.text_len == 0 ||
        parser->current.text[0] != '^') {
        return SE_HINT_NONE;
    }
    SeTypeHint hint = SE_HINT_NONE;
    if (parser->current.text_len == 3 && strncmp(parser->current.text, "^u8", 3) == 0) {
        hint = SE_HINT_U8;
    } else if (parser->current.text_len == 3 && strncmp(parser->current.text, "^i8", 3) == 0) {
        hint = SE_HINT_I8;
    } else if (parser->current.text_len == 4 && strncmp(parser->current.text, "^u16", 4) == 0) {
        hint = SE_HINT_U16;
    } else if (parser->current.text_len == 4 && strncmp(parser->current.text, "^i16", 4) == 0) {
        hint = SE_HINT_I16;
    }
    // Even for unknown hints, skip them
    advance(parser);
    return hint;
}

// Parse (def name value)
static AstNode* parse_def(SeParser* parser) {
    advance(parser); // skip 'def'

    if (parser->current.kind != SE_TOKEN_SYMBOL) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL, "def requires a name");
        return NULL;
    }

    AstNode* node = alloc_node(parser, AST_DEF);
    if (!node) return NULL;

    copy_token_text(node->as.def.name, &parser->current, SE_MAX_SYMBOL_LEN);
    advance(parser);

    node->as.def.value = se_parser_parse_form(parser);
    if (!node->as.def.value) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "def requires a value");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (defn name (params) body...)
static AstNode* parse_defn(SeParser* parser) {
    advance(parser); // skip 'defn'

    if (parser->current.kind != SE_TOKEN_SYMBOL) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL, "defn requires a name");
        return NULL;
    }

    AstNode* node = alloc_node(parser, AST_DEFN);
    if (!node) return NULL;

    copy_token_text(node->as.defn.name, &parser->current, SE_MAX_SYMBOL_LEN);
    advance(parser);

    // Parse parameter list
    if (parser->current.kind != SE_TOKEN_LPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_LPAREN, "defn requires parameter list");
        return NULL;
    }
    advance(parser);

    node->as.defn.param_count = 0;
    while (parser->current.kind == SE_TOKEN_SYMBOL) {
        if (node->as.defn.param_count >= SE_MAX_PARAMS) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_MANY_ARGS, "too many parameters");
            return NULL;
        }
        size_t idx = node->as.defn.param_count;
        // Optional ^hint before parameter name
        node->as.defn.param_hints[idx] = try_parse_hint(parser);
        if (parser->current.kind != SE_TOKEN_SYMBOL) break;
        copy_token_text(node->as.defn.params[idx], &parser->current, SE_MAX_SYMBOL_LEN);
        node->as.defn.param_count++;
        advance(parser);
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, "unclosed parameter list");
        return NULL;
    }
    advance(parser);

    // Parse body forms (dynamic array)
    ast_array_init(&node->as.defn.body);
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        if (!ast_array_push(&node->as.defn.body, form)) {
            parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY, "failed to add body form");
            return NULL;
        }
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

static AstNode* parse_fn(SeParser* parser) {
    advance(parser); // skip 'fn'

    AstNode* node = alloc_node(parser, AST_FN);
    if (!node) return NULL;

    // Generate unique name: __fn0, __fn1, etc.
    snprintf(node->as.defn.name, SE_MAX_SYMBOL_LEN, "__fn%d", parser->fn_counter++);

    // Parse parameter list
    if (parser->current.kind != SE_TOKEN_LPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_LPAREN, "fn requires parameter list");
        return NULL;
    }
    advance(parser);

    node->as.defn.param_count = 0;
    while (parser->current.kind == SE_TOKEN_SYMBOL) {
        if (node->as.defn.param_count >= SE_MAX_PARAMS) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_MANY_ARGS, "too many parameters");
            return NULL;
        }
        size_t idx = node->as.defn.param_count;
        // Optional ^hint before parameter name
        node->as.defn.param_hints[idx] = try_parse_hint(parser);
        if (parser->current.kind != SE_TOKEN_SYMBOL) break;
        copy_token_text(node->as.defn.params[idx], &parser->current, SE_MAX_SYMBOL_LEN);
        node->as.defn.param_count++;
        advance(parser);
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, "unclosed parameter list");
        return NULL;
    }
    advance(parser);

    // Parse body forms
    ast_array_init(&node->as.defn.body);
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        if (!ast_array_push(&node->as.defn.body, form)) {
            parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY, "failed to add body form");
            return NULL;
        }
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

static AstNode* parse_defmacro(SeParser* parser) {
    advance(parser);

    if (parser->current.kind != SE_TOKEN_SYMBOL) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL, "defmacro requires a name");
        return NULL;
    }

    AstNode* node = alloc_node(parser, AST_DEFMACRO);
    if (!node) return NULL;

    copy_token_text(node->as.defn.name, &parser->current, SE_MAX_SYMBOL_LEN);
    advance(parser);

    if (parser->current.kind != SE_TOKEN_LPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_LPAREN,
                         "defmacro requires parameter list");
        return NULL;
    }
    advance(parser);

    node->as.defn.param_count = 0;
    while (parser->current.kind == SE_TOKEN_SYMBOL) {
        if (node->as.defn.param_count >= SE_MAX_PARAMS) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_MANY_ARGS, "too many parameters");
            return NULL;
        }
        copy_token_text(node->as.defn.params[node->as.defn.param_count++], &parser->current,
                        SE_MAX_SYMBOL_LEN);
        advance(parser);
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, "unclosed parameter list");
        return NULL;
    }
    advance(parser);

    ast_array_init(&node->as.defn.body);
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        if (!ast_array_push(&node->as.defn.body, form)) {
            parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY, "failed to add body form");
            return NULL;
        }
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (let (var val var val ...) body...)
static AstNode* parse_let(SeParser* parser) {
    advance(parser); // skip 'let'

    if (parser->current.kind != SE_TOKEN_LPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_LPAREN, "let requires binding list");
        return NULL;
    }
    advance(parser);

    AstNode* node = alloc_node(parser, AST_LET);
    if (!node) return NULL;

    node->as.let.binding_count = 0;
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        if (node->as.let.binding_count >= SE_MAX_PARAMS) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_MANY_ARGS, "too many bindings");
            return NULL;
        }

        size_t idx = node->as.let.binding_count;

        // Optional ^hint before binding name
        node->as.let.hints[idx] = try_parse_hint(parser);

        if (parser->current.kind != SE_TOKEN_SYMBOL) {
            parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL,
                             "binding requires variable name");
            return NULL;
        }

        copy_token_text(node->as.let.vars[idx], &parser->current, SE_MAX_SYMBOL_LEN);
        advance(parser);

        node->as.let.vals[idx] = se_parser_parse_form(parser);
        if (!node->as.let.vals[idx]) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "binding requires value");
            return NULL;
        }

        node->as.let.binding_count++;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, "unclosed binding list");
        return NULL;
    }
    advance(parser);

    // Parse body forms (dynamic array)
    ast_array_init(&node->as.let.body);
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        if (!ast_array_push(&node->as.let.body, form)) {
            parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY, "failed to add body form");
            return NULL;
        }
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (var name value) or (var ^hint name value)
static AstNode* parse_var(SeParser* parser) {
    advance(parser); // skip 'var'

    // Optional ^hint
    SeTypeHint hint = try_parse_hint(parser);

    if (parser->current.kind != SE_TOKEN_SYMBOL) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL, "var requires variable name");
        return NULL;
    }

    AstNode* node = alloc_node(parser, AST_VAR);
    if (!node) return NULL;

    node->as.var.type_hint = hint;
    copy_token_text(node->as.var.name, &parser->current, SE_MAX_SYMBOL_LEN);
    advance(parser);

    node->as.var.value = se_parser_parse_form(parser);
    if (!node->as.var.value) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "var requires value");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (set! target value)
// target can be:
//   - symbol:  (set! x 10)
//   - field:   (set! (:field record) value)
static AstNode* parse_set_bang(SeParser* parser) {
    advance(parser); // skip 'set!'

    AstNode* node = alloc_node(parser, AST_SET_BANG);
    if (!node) return NULL;

    node->as.set.target_expr = NULL;

    // Check if target is (:field record-expr) or (nth array index)
    if (parser->current.kind == SE_TOKEN_LPAREN) {
        AstNode* target = se_parser_parse_form(parser);
        if (!target) return NULL;

        if (target->kind == AST_FIELD_GET) {
            // (set! (:field record) value) - field mutation
            strncpy(node->as.set.var, target->as.field_get.field, SE_MAX_SYMBOL_LEN - 1);
            node->as.set.var[SE_MAX_SYMBOL_LEN - 1] = '\0';
            node->as.set.target_expr = target->as.field_get.record;
        } else if (target->kind == AST_NTH) {
            // (set! (nth array index) value) - array element mutation
            node->as.set.var[0] = '\0'; // empty var signals nth target
            node->as.set.target_expr = target;
        } else {
            parser_set_error(parser, SE_PARSE_ERROR_UNEXPECTED_TOKEN,
                             "set! compound target must be (:field record) or (nth array index)");
            return NULL;
        }
    } else {
        if (parser->current.kind != SE_TOKEN_SYMBOL) {
            parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL, "set! requires variable name");
            return NULL;
        }
        copy_token_text(node->as.set.var, &parser->current, SE_MAX_SYMBOL_LEN);
        advance(parser);
    }

    node->as.set.value = se_parser_parse_form(parser);
    if (!node->as.set.value) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "set! requires value");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (if cond then else)
static AstNode* parse_if(SeParser* parser) {
    advance(parser); // skip 'if'

    AstNode* node = alloc_node(parser, AST_IF);
    if (!node) return NULL;

    node->as.if_expr.cond = se_parser_parse_form(parser);
    if (!node->as.if_expr.cond) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "if requires condition");
        return NULL;
    }

    node->as.if_expr.then_branch = se_parser_parse_form(parser);
    if (!node->as.if_expr.then_branch) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "if requires then branch");
        return NULL;
    }

    if (parser->current.kind == SE_TOKEN_RPAREN) {
        node->as.if_expr.else_branch = alloc_node(parser, AST_NIL);
    } else {
        node->as.if_expr.else_branch = se_parser_parse_form(parser);
        if (!node->as.if_expr.else_branch) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "if else branch parse failed");
            return NULL;
        }
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (while cond body...)
static AstNode* parse_while(SeParser* parser) {
    advance(parser); // skip 'while'

    AstNode* node = alloc_node(parser, AST_WHILE);
    if (!node) return NULL;

    node->as.while_expr.cond = se_parser_parse_form(parser);
    if (!node->as.while_expr.cond) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "while requires condition");
        return NULL;
    }

    // Parse body forms (dynamic array)
    ast_array_init(&node->as.while_expr.body);
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        if (!ast_array_push(&node->as.while_expr.body, form)) {
            parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY, "failed to add body form");
            return NULL;
        }
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (do expr...)
static AstNode* parse_do(SeParser* parser) {
    advance(parser); // skip 'do'

    AstNode* node = alloc_node(parser, AST_DO);
    if (!node) return NULL;

    // Parse expressions (dynamic array)
    ast_array_init(&node->as.block.exprs);
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        if (!ast_array_push(&node->as.block.exprs, form)) {
            parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY, "failed to add expression");
            return NULL;
        }
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (db expr...) - data block for (data name (db ...))
static AstNode* parse_db(SeParser* parser) {
    advance(parser); // skip 'db'

    AstNode* node = alloc_node(parser, AST_DB);
    if (!node) return NULL;

    ast_array_init(&node->as.block.exprs);
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        if (!ast_array_push(&node->as.block.exprs, form)) {
            parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY, "failed to add db value");
            return NULL;
        }
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (data name [addr] body...) - legacy data block
static AstNode* parse_data(SeParser* parser) {
    advance(parser); // skip 'data'

    if (parser->current.kind != SE_TOKEN_SYMBOL) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL, "data requires a name");
        return NULL;
    }

    AstNode* node = alloc_node(parser, AST_DATA);
    if (!node) return NULL;

    copy_token_text(node->as.data.name, &parser->current, SE_MAX_SYMBOL_LEN);
    advance(parser);

    node->as.data.addr = -1;
    node->as.data.addr_expr = NULL;

    ast_array_init(&node->as.data.body);
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        if (!ast_array_push(&node->as.data.body, form)) {
            parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY, "failed to add data body");
            return NULL;
        }
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (cond test1 body1 test2 body2 ...)
static AstNode* parse_cond(SeParser* parser) {
    advance(parser); // skip 'cond'

    AstNode* node = alloc_node(parser, AST_COND);
    if (!node) return NULL;

    node->as.cond.clause_count = 0;
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END &&
           node->as.cond.clause_count < SE_MAX_COND_CLAUSES) {

        size_t i = node->as.cond.clause_count;

        // Parse test
        node->as.cond.tests[i] = se_parser_parse_form(parser);
        if (!node->as.cond.tests[i]) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "cond requires test");
            return NULL;
        }

        // Parse body (at least one form required)
        ast_array_init(&node->as.cond.bodies[i]);
        if (parser->current.kind == SE_TOKEN_RPAREN || parser->current.kind == SE_TOKEN_END) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "cond test requires body");
            return NULL;
        }

        AstNode* body_form = se_parser_parse_form(parser);
        if (!body_form) return NULL;
        if (!ast_array_push(&node->as.cond.bodies[i], body_form)) {
            parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY, "cond body");
            return NULL;
        }

        node->as.cond.clause_count++;
    }

    if (node->as.cond.clause_count == 0) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS,
                         "cond requires at least one test-body pair");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (when cond body...)
static AstNode* parse_when(SeParser* parser) {
    advance(parser); // skip 'when'

    AstNode* node = alloc_node(parser, AST_WHEN);
    if (!node) return NULL;

    node->as.when_expr.cond = se_parser_parse_form(parser);
    if (!node->as.when_expr.cond) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "when requires condition");
        return NULL;
    }

    ast_array_init(&node->as.when_expr.body);
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        if (!ast_array_push(&node->as.when_expr.body, form)) {
            parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY, "when body");
            return NULL;
        }
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (unless cond body...)
static AstNode* parse_unless(SeParser* parser) {
    advance(parser); // skip 'unless'

    AstNode* node = alloc_node(parser, AST_UNLESS);
    if (!node) return NULL;

    node->as.when_expr.cond = se_parser_parse_form(parser);
    if (!node->as.when_expr.cond) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "unless requires condition");
        return NULL;
    }

    ast_array_init(&node->as.when_expr.body);
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        if (!ast_array_push(&node->as.when_expr.body, form)) {
            parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY, "unless body");
            return NULL;
        }
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (and a b ...) - build left-associative tree
static AstNode* parse_and(SeParser* parser) {
    advance(parser); // skip 'and'

    AstNode* left = se_parser_parse_form(parser);
    if (!left) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "and requires at least one form");
        return NULL;
    }

    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        AstNode* right = se_parser_parse_form(parser);
        if (!right) return NULL;
        AstNode* node = alloc_node(parser, AST_LOGIC_AND);
        if (!node) return NULL;
        node->as.binary.left = left;
        node->as.binary.right = right;
        left = node;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return left;
}

// Parse (or a b ...) - build left-associative tree
static AstNode* parse_or(SeParser* parser) {
    advance(parser); // skip 'or'

    AstNode* left = se_parser_parse_form(parser);
    if (!left) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "or requires at least one form");
        return NULL;
    }

    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        AstNode* right = se_parser_parse_form(parser);
        if (!right) return NULL;
        AstNode* node = alloc_node(parser, AST_LOGIC_OR);
        if (!node) return NULL;
        node->as.binary.left = left;
        node->as.binary.right = right;
        left = node;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return left;
}

// Parse (not expr)
static AstNode* parse_not(SeParser* parser) {
    advance(parser); // skip 'not'
    return parse_unary(parser, AST_LOGIC_NOT);
}

// Parse (range start end)
static AstNode* parse_range(SeParser* parser) {
    advance(parser); // skip 'range'

    AstNode* node = alloc_node(parser, AST_RANGE);
    if (!node) return NULL;

    node->as.range.start = se_parser_parse_form(parser);
    if (!node->as.range.start) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "range requires start");
        return NULL;
    }

    node->as.range.end = se_parser_parse_form(parser);
    if (!node->as.range.end) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "range requires end");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (for (binding collection) body...) or (for (binding coll :when cond) body...)
static AstNode* parse_for(SeParser* parser) {
    advance(parser); // skip 'for'

    if (parser->current.kind != SE_TOKEN_LPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_LPAREN,
                         "for requires (binding collection)");
        return NULL;
    }
    advance(parser);

    if (parser->current.kind != SE_TOKEN_SYMBOL) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL, "for binding must be symbol");
        return NULL;
    }

    AstNode* node = alloc_node(parser, AST_FOR);
    if (!node) return NULL;

    copy_token_text(node->as.for_expr.var, &parser->current, SE_MAX_SYMBOL_LEN);
    advance(parser);

    node->as.for_expr.collection = se_parser_parse_form(parser);
    if (!node->as.for_expr.collection) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "for requires collection");
        return NULL;
    }

    node->as.for_expr.when_cond = NULL;
    if (parser->current.kind == SE_TOKEN_KEYWORD) {
        size_t len = parser->current.text_len;
        int is_when = (len == 5 && strncmp(parser->current.text, ":when", 5) == 0);
        if (is_when) {
            advance(parser);
            node->as.for_expr.when_cond = se_parser_parse_form(parser);
        }
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, "for binding list");
        return NULL;
    }
    advance(parser);

    ast_array_init(&node->as.for_expr.body);
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        if (!ast_array_push(&node->as.for_expr.body, form)) {
            parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY, "for body");
            return NULL;
        }
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (ns name) or (ns name (require ...))
// Returns just the ns node; any require forms need separate handling
static AstNode* parse_ns(SeParser* parser) {
    advance(parser); // skip 'ns'

    if (parser->current.kind != SE_TOKEN_SYMBOL) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL, "ns requires a namespace symbol");
        return NULL;
    }

    AstNode* node = alloc_node(parser, AST_NS);
    if (!node) return NULL;

    copy_token_text(node->as.symbol.name, &parser->current, SE_MAX_SYMBOL_LEN);
    advance(parser);

    // Skip body forms - parse_ns_with_requires will handle them
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        if (parser->current.kind == SE_TOKEN_LPAREN) {
            // Skip nested form
            int depth = 1;
            advance(parser);
            while (depth > 0 && parser->current.kind != SE_TOKEN_END) {
                if (parser->current.kind == SE_TOKEN_LPAREN)
                    depth++;
                else if (parser->current.kind == SE_TOKEN_RPAREN)
                    depth--;
                advance(parser);
            }
        } else {
            advance(parser);
        }
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (require name...) or (require (name1 name2...))
static AstNode* parse_require(SeParser* parser) {
    advance(parser); // skip 'require'

    AstNode* node = alloc_node(parser, AST_REQUIRE);
    if (!node) return NULL;

    // Initialize dynamic array
    ast_array_init(&node->as.block.exprs);

    // Check if first element is a list (Clojure-style)
    if (parser->current.kind == SE_TOKEN_LPAREN) {
        advance(parser); // skip '('

        // Parse symbols/strings inside the list
        while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
            if (parser->current.kind != SE_TOKEN_SYMBOL &&
                parser->current.kind != SE_TOKEN_STRING) {
                parser_set_error(parser, SE_PARSE_ERROR_UNEXPECTED_TOKEN,
                                 "require expects a symbol or string");
                return NULL;
            }

            AstNode* entry = se_parser_parse_form(parser);
            if (!entry) return NULL;
            if (!ast_array_push(&node->as.block.exprs, entry)) {
                parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY,
                                 "failed to add require entry");
                return NULL;
            }
        }

        if (parser->current.kind != SE_TOKEN_RPAREN) {
            parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
            return NULL;
        }
        advance(parser); // skip ')'
    } else {
        // Original style: (require name1 name2...)
        while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
            if (parser->current.kind != SE_TOKEN_SYMBOL &&
                parser->current.kind != SE_TOKEN_STRING) {
                parser_set_error(parser, SE_PARSE_ERROR_UNEXPECTED_TOKEN,
                                 "require expects a symbol or string");
                return NULL;
            }

            AstNode* entry = se_parser_parse_form(parser);
            if (!entry) return NULL;
            if (!ast_array_push(&node->as.block.exprs, entry)) {
                parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY,
                                 "failed to add require entry");
                return NULL;
            }
        }
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse binary operation
static AstNode* parse_binary(SeParser* parser, AstKind kind) {
    advance(parser); // skip operator

    AstNode* node = alloc_node(parser, kind);
    if (!node) return NULL;

    node->as.binary.left = se_parser_parse_form(parser);
    if (!node->as.binary.left) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "binary op requires left operand");
        return NULL;
    }

    node->as.binary.right = se_parser_parse_form(parser);
    if (!node->as.binary.right) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "binary op requires right operand");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse unary operation
static AstNode* parse_unary(SeParser* parser, AstKind kind) {
    advance(parser); // skip operator

    AstNode* node = alloc_node(parser, kind);
    if (!node) return NULL;

    node->as.unary.operand = se_parser_parse_form(parser);
    if (!node->as.unary.operand) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "unary op requires operand");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (load addr-expr)
static AstNode* parse_load(SeParser* parser) {
    advance(parser); // skip 'load'

    AstNode* node = alloc_node(parser, AST_LOAD);
    if (!node) return NULL;

    node->as.load.addr = se_parser_parse_form(parser);
    if (!node->as.load.addr) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "load requires address");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (store addr-expr value)
static AstNode* parse_store(SeParser* parser) {
    advance(parser); // skip 'store'

    AstNode* node = alloc_node(parser, AST_STORE);
    if (!node) return NULL;

    node->as.store.addr = se_parser_parse_form(parser);
    if (!node->as.store.addr) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "store requires address");
        return NULL;
    }

    node->as.store.value = se_parser_parse_form(parser);
    if (!node->as.store.value) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "store requires value");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (defrecord name (field1 field2 ... fieldN))
// Fields may have ^hint annotations: (defrecord entity (^i16 x ^i16 y hp state))
static AstNode* parse_defrecord(SeParser* parser) {
    advance(parser); // skip 'defrecord'

    if (parser->current.kind != SE_TOKEN_SYMBOL) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL, "defrecord requires a name");
        return NULL;
    }

    AstNode* node = alloc_node(parser, AST_DEFRECORD);
    if (!node) return NULL;

    copy_token_text(node->as.defrecord.name, &parser->current, SE_MAX_SYMBOL_LEN);
    advance(parser);

    // Parse field list
    if (parser->current.kind != SE_TOKEN_LPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_LPAREN, "defrecord requires field list");
        return NULL;
    }
    advance(parser);

    node->as.defrecord.field_count = 0;
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        if (node->as.defrecord.field_count >= SE_MAX_PARAMS) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_MANY_ARGS, "too many record fields");
            return NULL;
        }

        size_t idx = node->as.defrecord.field_count;
        node->as.defrecord.field_is_16bit[idx] = false;
        node->as.defrecord.field_is_signed[idx] = false;

        // Check for ^hint annotation
        if (parser->current.kind == SE_TOKEN_SYMBOL && parser->current.text_len > 0 &&
            parser->current.text[0] == '^') {
            // Check if it's ^i16 or ^u16
            if ((parser->current.text_len == 4 &&
                 (strncmp(parser->current.text, "^i16", 4) == 0 ||
                  strncmp(parser->current.text, "^u16", 4) == 0))) {
                node->as.defrecord.field_is_16bit[idx] = true;
            }
            // Check if it's ^i16 or ^i8 (signed)
            if ((parser->current.text_len == 4 && (strncmp(parser->current.text, "^i16", 4) == 0 ||
                                                   strncmp(parser->current.text, "^i8", 3) == 0))) {
                node->as.defrecord.field_is_signed[idx] = true;
            }
            advance(parser); // skip hint
        }

        if (parser->current.kind != SE_TOKEN_SYMBOL) {
            parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL, "expected field name");
            return NULL;
        }

        copy_token_text(node->as.defrecord.fields[idx], &parser->current, SE_MAX_SYMBOL_LEN);
        advance(parser);
        node->as.defrecord.field_count++;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, "unclosed field list");
        return NULL;
    }
    advance(parser);

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (array count value)
static AstNode* parse_array(SeParser* parser) {
    advance(parser); // skip 'array'

    AstNode* node = alloc_node(parser, AST_ARRAY);
    if (!node) return NULL;

    node->as.array_expr.count = se_parser_parse_form(parser);
    if (!node->as.array_expr.count) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "array requires count");
        return NULL;
    }

    node->as.array_expr.value = se_parser_parse_form(parser);
    if (!node->as.array_expr.value) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "array requires initial value");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (nth array-expr index-expr) - reuses binary layout
static AstNode* parse_nth(SeParser* parser) {
    advance(parser); // skip 'nth'

    AstNode* node = alloc_node(parser, AST_NTH);
    if (!node) return NULL;

    node->as.binary.left = se_parser_parse_form(parser);
    if (!node->as.binary.left) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "nth requires array");
        return NULL;
    }

    node->as.binary.right = se_parser_parse_form(parser);
    if (!node->as.binary.right) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "nth requires index");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (len array-expr) - reuses unary layout
static AstNode* parse_len(SeParser* parser) {
    advance(parser); // skip 'len'

    AstNode* node = alloc_node(parser, AST_LEN);
    if (!node) return NULL;

    node->as.unary.operand = se_parser_parse_form(parser);
    if (!node->as.unary.operand) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "len requires array");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (include "filename")
static AstNode* parse_import(SeParser* parser) {
    advance(parser); // skip 'include'

    if (parser->current.kind != SE_TOKEN_STRING) {
        parser_set_error(parser, SE_PARSE_ERROR_UNEXPECTED_TOKEN,
                         "include requires string filename");
        return NULL;
    }

    AstNode* node = alloc_node(parser, AST_IMPORT);
    if (!node) return NULL;

    // Copy filename without quotes
    size_t len = parser->current.text_len;
    if (len >= 2) {
        len -= 2;
        if (len >= SE_MAX_SYMBOL_LEN) len = SE_MAX_SYMBOL_LEN - 1;
        memcpy(node->as.symbol.name, parser->current.text + 1, len);
        node->as.symbol.name[len] = '\0';
    }
    advance(parser);

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (asm "raw assembly...")
static AstNode* parse_asm(SeParser* parser) {
    advance(parser); // skip 'asm'

    if (parser->current.kind != SE_TOKEN_STRING) {
        parser_set_error(parser, SE_PARSE_ERROR_UNEXPECTED_TOKEN, "asm requires string");
        return NULL;
    }

    AstNode* node = alloc_node(parser, AST_ASM);
    if (!node) return NULL;

    // Copy assembly without quotes
    size_t len = parser->current.text_len;
    if (len >= 2) {
        len -= 2;
        if (len >= SE_MAX_SYMBOL_LEN) len = SE_MAX_SYMBOL_LEN - 1;
        memcpy(node->as.symbol.name, parser->current.text + 1, len);
        node->as.symbol.name[len] = '\0';
    }
    advance(parser);

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse function call (func args...)
static AstNode* parse_call(SeParser* parser) {
    AstNode* node = alloc_node(parser, AST_CALL);
    if (!node) return NULL;

    copy_token_text(node->as.call.func, &parser->current, SE_MAX_SYMBOL_LEN);
    advance(parser);

    node->as.call.arg_count = 0;
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        if (node->as.call.arg_count >= SE_MAX_PARAMS) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_MANY_ARGS, "too many arguments");
            return NULL;
        }
        AstNode* arg = se_parser_parse_form(parser);
        if (!arg) return NULL;
        node->as.call.args[node->as.call.arg_count++] = arg;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

static AstNode* parse_list(SeParser* parser) {
    advance(parser); // skip '('

    if (parser->current.kind == SE_TOKEN_RPAREN) {
        // Empty list - treat as nil/0
        advance(parser);
        AstNode* node = alloc_node(parser, AST_NUMBER);
        if (node) node->as.number = 0;
        return node;
    }

    // Keyword-as-accessor: (:field record-expr)
    if (parser->current.kind == SE_TOKEN_KEYWORD) {
        AstNode* node = alloc_node(parser, AST_FIELD_GET);
        if (!node) return NULL;

        copy_token_text(node->as.field_get.field, &parser->current, SE_MAX_SYMBOL_LEN);
        advance(parser);

        node->as.field_get.record = se_parser_parse_form(parser);
        if (!node->as.field_get.record) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS,
                             "keyword accessor requires record expression");
            return NULL;
        }

        if (parser->current.kind != SE_TOKEN_RPAREN) {
            parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
            return NULL;
        }
        advance(parser);

        return node;
    }

    if (parser->current.kind != SE_TOKEN_SYMBOL) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL, "list must start with symbol");
        return NULL;
    }

    if (is_symbol(parser, "def")) return parse_def(parser);
    if (is_symbol(parser, "defn")) return parse_defn(parser);
    if (is_symbol(parser, "defmacro")) return parse_defmacro(parser);
    if (is_symbol(parser, "fn")) return parse_fn(parser);
    if (is_symbol(parser, "defrecord")) return parse_defrecord(parser);
    if (is_symbol(parser, "array")) return parse_array(parser);
    if (is_symbol(parser, "nth")) return parse_nth(parser);
    if (is_symbol(parser, "len")) return parse_len(parser);
    if (is_symbol(parser, "let")) return parse_let(parser);
    if (is_symbol(parser, "var")) return parse_var(parser);
    if (is_symbol(parser, "set!")) return parse_set_bang(parser);
    if (is_symbol(parser, "if")) return parse_if(parser);
    if (is_symbol(parser, "while")) return parse_while(parser);
    if (is_symbol(parser, "do")) return parse_do(parser);
    if (is_symbol(parser, "cond")) return parse_cond(parser);
    if (is_symbol(parser, "when")) return parse_when(parser);
    if (is_symbol(parser, "unless")) return parse_unless(parser);
    if (is_symbol(parser, "for")) return parse_for(parser);
    if (is_symbol(parser, "range")) return parse_range(parser);
    if (is_symbol(parser, "ns")) return parse_ns(parser);
    if (is_symbol(parser, "require")) return parse_require(parser);

    // Arithmetic
    if (is_symbol(parser, "+") || is_symbol(parser, "add")) return parse_binary(parser, AST_ADD);
    if (is_symbol(parser, "-") || is_symbol(parser, "sub")) {
        // Could be binary sub or unary neg
        (void)se_lexer_peek(&parser->lexer); // peek not needed, just checking next form
        advance(parser);                     // skip '-' or 'sub'
        AstNode* first = se_parser_parse_form(parser);
        if (!first) return NULL;

        if (parser->current.kind == SE_TOKEN_RPAREN) {
            // Unary negation
            AstNode* node = alloc_node(parser, AST_NEG);
            if (!node) return NULL;
            node->as.unary.operand = first;
            advance(parser);
            return node;
        } else {
            // Binary subtraction
            AstNode* node = alloc_node(parser, AST_SUB);
            if (!node) return NULL;
            node->as.binary.left = first;
            node->as.binary.right = se_parser_parse_form(parser);
            if (!node->as.binary.right) return NULL;
            if (parser->current.kind != SE_TOKEN_RPAREN) {
                parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
                return NULL;
            }
            advance(parser);
            return node;
        }
    }
    if (is_symbol(parser, "neg")) return parse_unary(parser, AST_NEG);
    if (is_symbol(parser, "inc")) return parse_unary(parser, AST_INC);
    if (is_symbol(parser, "dec")) return parse_unary(parser, AST_DEC);

    // Bitwise
    if (is_symbol(parser, "&")) return parse_binary(parser, AST_BAND);
    if (is_symbol(parser, "|")) return parse_binary(parser, AST_BOR);
    if (is_symbol(parser, "^") || is_symbol(parser, "xor")) return parse_binary(parser, AST_XOR);
    if (is_symbol(parser, "~")) return parse_unary(parser, AST_BNOT);
    // Logical (short-circuit)
    if (is_symbol(parser, "and")) return parse_and(parser);
    if (is_symbol(parser, "or")) return parse_or(parser);
    if (is_symbol(parser, "not")) return parse_not(parser);
    if (is_symbol(parser, "<<") || is_symbol(parser, "shl")) return parse_binary(parser, AST_SHL);
    if (is_symbol(parser, ">>") || is_symbol(parser, "shr")) return parse_binary(parser, AST_SHR);
    if (is_symbol(parser, "*") || is_symbol(parser, "mul")) return parse_binary(parser, AST_MUL);
    if (is_symbol(parser, "/") || is_symbol(parser, "div")) return parse_binary(parser, AST_DIV);
    if (is_symbol(parser, "%") || is_symbol(parser, "mod")) return parse_binary(parser, AST_MOD);

    // Comparison
    if (is_symbol(parser, "=") || is_symbol(parser, "eq")) return parse_binary(parser, AST_EQ);
    if (is_symbol(parser, "!=") || is_symbol(parser, "ne")) return parse_binary(parser, AST_NE);
    if (is_symbol(parser, "<") || is_symbol(parser, "lt")) return parse_binary(parser, AST_LT);
    if (is_symbol(parser, ">") || is_symbol(parser, "gt")) return parse_binary(parser, AST_GT);
    if (is_symbol(parser, "<=") || is_symbol(parser, "le")) return parse_binary(parser, AST_LE);
    if (is_symbol(parser, ">=") || is_symbol(parser, "ge")) return parse_binary(parser, AST_GE);
    if (is_symbol(parser, "!") || is_symbol(parser, "lnot")) return parse_unary(parser, AST_LNOT);

    // Type predicates
    if (is_symbol(parser, "nil?")) return parse_unary(parser, AST_NILP);
    if (is_symbol(parser, "zero?")) return parse_unary(parser, AST_ZEROP);
    if (is_symbol(parser, "pos?")) return parse_unary(parser, AST_POSP);
    if (is_symbol(parser, "neg?")) return parse_unary(parser, AST_NEGP);

    // Type casts
    if (is_symbol(parser, "u8")) return parse_unary(parser, AST_CAST_U8);
    if (is_symbol(parser, "i8")) return parse_unary(parser, AST_CAST_I8);

    // Memory - load/store take single 16-bit address expression
    if (is_symbol(parser, "load")) return parse_load(parser);
    if (is_symbol(parser, "store")) return parse_store(parser);

    // Compile-time helpers
    if (is_symbol(parser, "hi")) return parse_unary(parser, AST_HI);
    if (is_symbol(parser, "lo")) return parse_unary(parser, AST_LO);

    // Special directives
    if (is_symbol(parser, "import")) return parse_import(parser);
    if (is_symbol(parser, "asm")) return parse_asm(parser);

    // Legacy data: (data name body...) and (db expr...)
    if (is_symbol(parser, "data")) return parse_data(parser);
    if (is_symbol(parser, "db")) return parse_db(parser);

    // Otherwise, it's a function call
    return parse_call(parser);
}

// Parse (ns name (require (modules...))) and extract forms
bool se_parser_parse_ns_with_requires(SeParser* parser, AstProgram* program) {
    if (parser->current.kind != SE_TOKEN_LPAREN) return false;

    SeToken peek = se_lexer_peek(&parser->lexer);
    if (peek.kind != SE_TOKEN_SYMBOL || !se_token_is_symbol(&peek, "ns")) {
        return false;
    }

    advance(parser); // skip '('
    advance(parser); // skip 'ns'

    if (parser->current.kind != SE_TOKEN_SYMBOL) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL, "ns requires a namespace symbol");
        return true; // consumed ns form
    }

    AstNode* ns_node = alloc_node(parser, AST_NS);
    if (!ns_node) return true;
    copy_token_text(ns_node->as.symbol.name, &parser->current, SE_MAX_SYMBOL_LEN);
    advance(parser);

    if (!ast_program_add(program, ns_node)) {
        parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY, "failed to add ns node");
        return true;
    }

    // Parse body forms (mainly require)
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return true;
        if (!ast_program_add(program, form)) {
            parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY, "failed to add form");
            return true;
        }
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
    }
    advance(parser);

    return true; // consumed ns form
}

bool se_parser_parse_program(SeParser* parser, AstProgram* program) {
    ast_program_init(program);

    while (parser->current.kind != SE_TOKEN_END && parser->error == SE_PARSE_OK) {
        // Try to parse (ns ...) with nested forms
        if (se_parser_parse_ns_with_requires(parser, program)) {
            continue;
        }

        AstNode* form = se_parser_parse_form(parser);
        if (!form) break;

        if (!ast_program_add(program, form)) {
            parser_set_error(parser, SE_PARSE_ERROR_OUT_OF_MEMORY, "failed to add top-level form");
            return false;
        }
    }

    return parser->error == SE_PARSE_OK;
}

bool se_parser_has_error(SeParser* parser) { return parser->error != SE_PARSE_OK; }

void se_parser_print_error(SeParser* parser, const char* filename) {
    fprintf(stderr, "%s:%zu:%zu: error: %s", filename, parser->error_line, parser->error_column,
            se_parse_error_name(parser->error));
    if (parser->error_msg[0]) {
        fprintf(stderr, ": %s", parser->error_msg);
    }
    fprintf(stderr, "\n");
}
