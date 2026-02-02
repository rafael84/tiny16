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
    advance(parser);
}

static AstNode* parse_list(SeParser* parser);

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
        copy_token_text(node->as.defn.params[node->as.defn.param_count++], &parser->current,
                        SE_MAX_SYMBOL_LEN);
        advance(parser);
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, "unclosed parameter list");
        return NULL;
    }
    advance(parser);

    // Parse body forms
    node->as.defn.body_count = 0;
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        if (node->as.defn.body_count >= SE_MAX_CHILDREN) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_MANY_ARGS, "too many body expressions");
            return NULL;
        }
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        node->as.defn.body[node->as.defn.body_count++] = form;
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

        if (parser->current.kind != SE_TOKEN_SYMBOL) {
            parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL,
                             "binding requires variable name");
            return NULL;
        }

        size_t idx = node->as.let.binding_count;
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

    // Parse body forms
    node->as.let.body_count = 0;
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        if (node->as.let.body_count >= SE_MAX_CHILDREN) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_MANY_ARGS, "too many body expressions");
            return NULL;
        }
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        node->as.let.body[node->as.let.body_count++] = form;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (set var value)
static AstNode* parse_set(SeParser* parser) {
    advance(parser); // skip 'set'

    if (parser->current.kind != SE_TOKEN_SYMBOL) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL, "set requires variable name");
        return NULL;
    }

    AstNode* node = alloc_node(parser, AST_SET);
    if (!node) return NULL;

    copy_token_text(node->as.set.var, &parser->current, SE_MAX_SYMBOL_LEN);
    advance(parser);

    node->as.set.value = se_parser_parse_form(parser);
    if (!node->as.set.value) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "set requires value");
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

    node->as.if_expr.else_branch = se_parser_parse_form(parser);
    if (!node->as.if_expr.else_branch) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "if requires else branch");
        return NULL;
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

    node->as.while_expr.body_count = 0;
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        if (node->as.while_expr.body_count >= SE_MAX_CHILDREN) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_MANY_ARGS, "too many body expressions");
            return NULL;
        }
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        node->as.while_expr.body[node->as.while_expr.body_count++] = form;
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

    node->as.block.expr_count = 0;
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        if (node->as.block.expr_count >= SE_MAX_CHILDREN) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_MANY_ARGS, "too many expressions");
            return NULL;
        }
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        node->as.block.exprs[node->as.block.expr_count++] = form;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (data name [addr] body...)
static AstNode* parse_data(SeParser* parser) {
    advance(parser); // skip 'data'

    if (parser->current.kind != SE_TOKEN_SYMBOL) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL, "data requires name");
        return NULL;
    }

    AstNode* node = alloc_node(parser, AST_DATA);
    if (!node) return NULL;

    copy_token_text(node->as.data.name, &parser->current, SE_MAX_SYMBOL_LEN);
    advance(parser);

    // Optional address - can be a number, symbol, or arithmetic expression
    // But NOT special forms like (db ...), (repeat ...), etc.
    node->as.data.addr = -1;
    node->as.data.addr_expr = NULL;
    if (parser->current.kind == SE_TOKEN_NUMBER) {
        node->as.data.addr = parser->current.number_value;
        advance(parser);
    } else if (parser->current.kind == SE_TOKEN_SYMBOL) {
        // Symbol could be a constant/data label for address
        node->as.data.addr_expr = se_parser_parse_form(parser);
    } else if (parser->current.kind == SE_TOKEN_LPAREN) {
        // Peek inside: if it's an arithmetic op, parse as address
        SeToken peek = se_lexer_peek(&parser->lexer);
        if (peek.kind == SE_TOKEN_SYMBOL &&
            (se_token_is_symbol(&peek, "+") || se_token_is_symbol(&peek, "-") ||
             se_token_is_symbol(&peek, "*") || se_token_is_symbol(&peek, "/") ||
             se_token_is_symbol(&peek, "hi") || se_token_is_symbol(&peek, "lo") ||
             se_token_is_symbol(&peek, "&") || se_token_is_symbol(&peek, "|") ||
             se_token_is_symbol(&peek, "<<") || se_token_is_symbol(&peek, ">>"))) {
            node->as.data.addr_expr = se_parser_parse_form(parser);
        }
        // Otherwise, it's body content - don't consume it
    }

    // Parse body
    node->as.data.body_count = 0;
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        if (node->as.data.body_count >= SE_MAX_CHILDREN) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_MANY_ARGS, "too many data forms");
            return NULL;
        }
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        node->as.data.body[node->as.data.body_count++] = form;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (db values...)
static AstNode* parse_db(SeParser* parser) {
    advance(parser); // skip 'db'

    AstNode* node = alloc_node(parser, AST_DB);
    if (!node) return NULL;

    node->as.block.expr_count = 0;
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        if (node->as.block.expr_count >= SE_MAX_CHILDREN) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_MANY_ARGS, "too many values");
            return NULL;
        }
        AstNode* form = se_parser_parse_form(parser);
        if (!form) return NULL;
        node->as.block.exprs[node->as.block.expr_count++] = form;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (repeat count form)
static AstNode* parse_repeat(SeParser* parser) {
    advance(parser); // skip 'repeat'

    AstNode* node = alloc_node(parser, AST_REPEAT);
    if (!node) return NULL;

    if (parser->current.kind != SE_TOKEN_NUMBER) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_NUMBER, "repeat requires count");
        return NULL;
    }
    node->as.repeat.count = parser->current.number_value;
    advance(parser);

    node->as.repeat.form = se_parser_parse_form(parser);
    if (!node->as.repeat.form) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "repeat requires form");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (ns name)
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

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (require name...)
static AstNode* parse_require(SeParser* parser) {
    advance(parser); // skip 'require'

    AstNode* node = alloc_node(parser, AST_REQUIRE);
    if (!node) return NULL;

    node->as.block.expr_count = 0;
    while (parser->current.kind != SE_TOKEN_RPAREN && parser->current.kind != SE_TOKEN_END) {
        if (node->as.block.expr_count >= SE_MAX_CHILDREN) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_MANY_ARGS, "too many require entries");
            return NULL;
        }
        if (parser->current.kind != SE_TOKEN_SYMBOL && parser->current.kind != SE_TOKEN_STRING) {
            parser_set_error(parser, SE_PARSE_ERROR_UNEXPECTED_TOKEN,
                             "require expects a symbol or string");
            return NULL;
        }

        AstNode* entry = se_parser_parse_form(parser);
        if (!entry) return NULL;
        node->as.block.exprs[node->as.block.expr_count++] = entry;
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

// Parse (addr hi lo)
static AstNode* parse_addr(SeParser* parser) {
    advance(parser); // skip 'addr'

    AstNode* node = alloc_node(parser, AST_ADDR);
    if (!node) return NULL;

    node->as.addr.hi = se_parser_parse_form(parser);
    if (!node->as.addr.hi) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "addr requires hi byte");
        return NULL;
    }

    node->as.addr.lo = se_parser_parse_form(parser);
    if (!node->as.addr.lo) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "addr requires lo byte");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return node;
}

// Parse (addr16 value) - 16-bit address that auto-splits to hi/lo
static AstNode* parse_addr16(SeParser* parser) {
    advance(parser); // skip 'addr16'

    AstNode* node = alloc_node(parser, AST_ADDR16);
    if (!node) return NULL;

    node->as.unary.operand = se_parser_parse_form(parser);
    if (!node->as.unary.operand) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "addr16 requires address value");
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

// Parse (peek hi lo) - syntactic sugar for (load (addr hi lo))
static AstNode* parse_peek(SeParser* parser) {
    advance(parser); // skip 'peek'

    // Create inner addr node
    AstNode* addr_node = alloc_node(parser, AST_ADDR);
    if (!addr_node) return NULL;

    addr_node->as.addr.hi = se_parser_parse_form(parser);
    if (!addr_node->as.addr.hi) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "peek requires hi byte");
        return NULL;
    }

    addr_node->as.addr.lo = se_parser_parse_form(parser);
    if (!addr_node->as.addr.lo) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "peek requires lo byte");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    // Wrap in load node
    AstNode* load_node = alloc_node(parser, AST_LOAD);
    if (!load_node) return NULL;
    load_node->as.load.addr = addr_node;

    return load_node;
}

// Parse (peek16 addr) - syntactic sugar for (load (addr16 addr))
static AstNode* parse_peek16(SeParser* parser) {
    advance(parser); // skip 'peek16'

    // Create inner addr16 node
    AstNode* addr16_node = alloc_node(parser, AST_ADDR16);
    if (!addr16_node) return NULL;

    addr16_node->as.unary.operand = se_parser_parse_form(parser);
    if (!addr16_node->as.unary.operand) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "peek16 requires address");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    // Wrap in load node
    AstNode* load_node = alloc_node(parser, AST_LOAD);
    if (!load_node) return NULL;
    load_node->as.load.addr = addr16_node;

    return load_node;
}

// Parse (peek* hi lo) - syntactic sugar for (load (addr16 (addr hi lo)))
// This dereferences a pointer stored at [hi:lo]
static AstNode* parse_peek_ptr(SeParser* parser) {
    advance(parser); // skip 'peek*'

    // Create innermost addr node
    AstNode* addr_node = alloc_node(parser, AST_ADDR);
    if (!addr_node) return NULL;

    addr_node->as.addr.hi = se_parser_parse_form(parser);
    if (!addr_node->as.addr.hi) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "peek* requires hi byte");
        return NULL;
    }

    addr_node->as.addr.lo = se_parser_parse_form(parser);
    if (!addr_node->as.addr.lo) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "peek* requires lo byte");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    // Wrap addr in addr16 (to dereference the pointer)
    AstNode* addr16_node = alloc_node(parser, AST_ADDR16);
    if (!addr16_node) return NULL;
    addr16_node->as.unary.operand = addr_node;

    // Wrap in load node
    AstNode* load_node = alloc_node(parser, AST_LOAD);
    if (!load_node) return NULL;
    load_node->as.load.addr = addr16_node;

    return load_node;
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

// Parse (poke hi lo value) - syntactic sugar for (store (addr hi lo) value)
static AstNode* parse_poke(SeParser* parser) {
    advance(parser); // skip 'poke'

    // Create inner addr node
    AstNode* addr_node = alloc_node(parser, AST_ADDR);
    if (!addr_node) return NULL;

    addr_node->as.addr.hi = se_parser_parse_form(parser);
    if (!addr_node->as.addr.hi) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "poke requires hi byte");
        return NULL;
    }

    addr_node->as.addr.lo = se_parser_parse_form(parser);
    if (!addr_node->as.addr.lo) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "poke requires lo byte");
        return NULL;
    }

    // Create store node
    AstNode* store_node = alloc_node(parser, AST_STORE);
    if (!store_node) return NULL;
    store_node->as.store.addr = addr_node;

    store_node->as.store.value = se_parser_parse_form(parser);
    if (!store_node->as.store.value) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "poke requires value");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return store_node;
}

// Parse (poke16 addr value) - syntactic sugar for (store (addr16 addr) value)
static AstNode* parse_poke16(SeParser* parser) {
    advance(parser); // skip 'poke16'

    // Create inner addr16 node
    AstNode* addr16_node = alloc_node(parser, AST_ADDR16);
    if (!addr16_node) return NULL;

    addr16_node->as.unary.operand = se_parser_parse_form(parser);
    if (!addr16_node->as.unary.operand) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "poke16 requires address");
        return NULL;
    }

    // Create store node
    AstNode* store_node = alloc_node(parser, AST_STORE);
    if (!store_node) return NULL;
    store_node->as.store.addr = addr16_node;

    store_node->as.store.value = se_parser_parse_form(parser);
    if (!store_node->as.store.value) {
        parser_set_error(parser, SE_PARSE_ERROR_TOO_FEW_ARGS, "poke16 requires value");
        return NULL;
    }

    if (parser->current.kind != SE_TOKEN_RPAREN) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_RPAREN, NULL);
        return NULL;
    }
    advance(parser);

    return store_node;
}

// Parse (include "filename")
static AstNode* parse_include(SeParser* parser) {
    advance(parser); // skip 'include'

    if (parser->current.kind != SE_TOKEN_STRING) {
        parser_set_error(parser, SE_PARSE_ERROR_UNEXPECTED_TOKEN,
                         "include requires string filename");
        return NULL;
    }

    AstNode* node = alloc_node(parser, AST_INCLUDE);
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

    if (parser->current.kind != SE_TOKEN_SYMBOL) {
        parser_set_error(parser, SE_PARSE_ERROR_EXPECTED_SYMBOL, "list must start with symbol");
        return NULL;
    }

    if (is_symbol(parser, "def")) return parse_def(parser);
    if (is_symbol(parser, "defn")) return parse_defn(parser);
    if (is_symbol(parser, "let")) return parse_let(parser);
    if (is_symbol(parser, "set")) return parse_set(parser);
    if (is_symbol(parser, "if")) return parse_if(parser);
    if (is_symbol(parser, "while")) return parse_while(parser);
    if (is_symbol(parser, "do")) return parse_do(parser);
    if (is_symbol(parser, "data")) return parse_data(parser);
    if (is_symbol(parser, "db")) return parse_db(parser);
    if (is_symbol(parser, "repeat")) return parse_repeat(parser);
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
    if (is_symbol(parser, "&") || is_symbol(parser, "and")) return parse_binary(parser, AST_AND);
    if (is_symbol(parser, "|") || is_symbol(parser, "or")) return parse_binary(parser, AST_OR);
    if (is_symbol(parser, "^") || is_symbol(parser, "xor")) return parse_binary(parser, AST_XOR);
    if (is_symbol(parser, "~") || is_symbol(parser, "not")) return parse_unary(parser, AST_NOT);
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

    // Memory
    if (is_symbol(parser, "addr")) return parse_addr(parser);
    if (is_symbol(parser, "addr16")) return parse_addr16(parser);
    if (is_symbol(parser, "load")) return parse_load(parser);
    if (is_symbol(parser, "store")) return parse_store(parser);
    if (is_symbol(parser, "peek")) return parse_peek(parser);
    if (is_symbol(parser, "peek16")) return parse_peek16(parser);
    if (is_symbol(parser, "peek*")) return parse_peek_ptr(parser);
    if (is_symbol(parser, "poke")) return parse_poke(parser);
    if (is_symbol(parser, "poke16")) return parse_poke16(parser);

    // Compile-time helpers
    if (is_symbol(parser, "hi")) return parse_unary(parser, AST_HI);
    if (is_symbol(parser, "lo")) return parse_unary(parser, AST_LO);

    // Special directives
    if (is_symbol(parser, "include")) return parse_include(parser);
    if (is_symbol(parser, "asm")) return parse_asm(parser);

    // Otherwise, it's a function call
    return parse_call(parser);
}

bool se_parser_parse_program(SeParser* parser, AstProgram* program) {
    program->node_count = 0;

    while (parser->current.kind != SE_TOKEN_END && parser->error == SE_PARSE_OK) {
        if (program->node_count >= SE_MAX_FUNCTIONS + SE_MAX_CONSTANTS) {
            parser_set_error(parser, SE_PARSE_ERROR_TOO_MANY_ARGS, "too many top-level forms");
            return false;
        }

        AstNode* form = se_parser_parse_form(parser);
        if (!form) break;

        program->nodes[program->node_count++] = form;
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
