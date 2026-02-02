#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define realpath(N, R) _fullpath((R), (N), PATH_MAX)
#endif

// PATH_MAX may not be defined on all systems
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include "args.c"
#include "args.h"
#include "ast.c"
#include "ast.h"
#include "codegen.c"
#include "codegen.h"
#include "lexer.c"
#include "lexer.h"
#include "parser.c"
#include "parser.h"

static char* read_file(const char* filename, size_t* out_len);
static bool parse_with_requires(const char* filename, AstPool* pool, AstProgram* program);

#define SE_MAX_MODULES     128
#define SE_MAX_SCOPE_NAMES 128

typedef struct {
    char items[SE_MAX_MODULES][PATH_MAX];
    size_t count;
} SeModuleList;

typedef struct {
    const char* names[SE_MAX_SCOPE_NAMES];
    size_t count;
} SeScope;

static bool module_list_contains(SeModuleList* list, const char* item) {
    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->items[i], item) == 0) return true;
    }
    return false;
}

static bool module_list_add(SeModuleList* list, const char* item) {
    if (list->count >= SE_MAX_MODULES) return false;
    strncpy(list->items[list->count], item, PATH_MAX - 1);
    list->items[list->count][PATH_MAX - 1] = '\0';
    list->count++;
    return true;
}

static void scope_push(SeScope* scope, const char* name) {
    if (scope->count >= SE_MAX_SCOPE_NAMES) return;
    scope->names[scope->count++] = name;
}

static void scope_pop_to(SeScope* scope, size_t count) {
    if (count <= scope->count) scope->count = count;
}

static bool scope_contains(SeScope* scope, const char* name) {
    for (size_t i = 0; i < scope->count; i++) {
        if (strcmp(scope->names[i], name) == 0) return true;
    }
    return false;
}

static bool is_qualified_symbol(const char* name) { return strchr(name, '/') != NULL; }

static void qualify_name(char* name, const char* ns) {
    if (!ns || !*ns) return;
    if (is_qualified_symbol(name)) return;
    char buf[SE_MAX_SYMBOL_LEN];
    snprintf(buf, sizeof(buf), "%s/%s", ns, name);
    strncpy(name, buf, SE_MAX_SYMBOL_LEN - 1);
    name[SE_MAX_SYMBOL_LEN - 1] = '\0';
}

static void apply_namespace(AstNode* node, const char* ns, SeScope* scope);

static void apply_namespace_list(AstNode** nodes, size_t count, const char* ns, SeScope* scope) {
    for (size_t i = 0; i < count; i++) {
        apply_namespace(nodes[i], ns, scope);
    }
}

static void apply_namespace(AstNode* node, const char* ns, SeScope* scope) {
    if (!node) return;

    switch (node->kind) {
    case AST_NUMBER:
    case AST_STRING:
    case AST_ASM:
    case AST_IMPORT:
    case AST_NS:
    case AST_REQUIRE: return;

    case AST_SYMBOL:
        if (!scope_contains(scope, node->as.symbol.name)) {
            qualify_name(node->as.symbol.name, ns);
        }
        return;

    case AST_DEF:
        qualify_name(node->as.def.name, ns);
        apply_namespace(node->as.def.value, ns, scope);
        return;

    case AST_DEFN: {
        qualify_name(node->as.defn.name, ns);
        size_t saved = scope->count;
        for (size_t i = 0; i < node->as.defn.param_count; i++) {
            scope_push(scope, node->as.defn.params[i]);
        }
        apply_namespace_list(node->as.defn.body, node->as.defn.body_count, ns, scope);
        scope_pop_to(scope, saved);
        return;
    }

    case AST_LET: {
        size_t saved = scope->count;
        for (size_t i = 0; i < node->as.let.binding_count; i++) {
            apply_namespace(node->as.let.vals[i], ns, scope);
            scope_push(scope, node->as.let.vars[i]);
        }
        apply_namespace_list(node->as.let.body, node->as.let.body_count, ns, scope);
        scope_pop_to(scope, saved);
        return;
    }

    case AST_SET: apply_namespace(node->as.set.value, ns, scope); return;

    case AST_IF:
        apply_namespace(node->as.if_expr.cond, ns, scope);
        apply_namespace(node->as.if_expr.then_branch, ns, scope);
        apply_namespace(node->as.if_expr.else_branch, ns, scope);
        return;

    case AST_WHILE:
        apply_namespace(node->as.while_expr.cond, ns, scope);
        apply_namespace_list(node->as.while_expr.body, node->as.while_expr.body_count, ns, scope);
        return;

    case AST_DO:
    case AST_DB:
        apply_namespace_list(node->as.block.exprs, node->as.block.expr_count, ns, scope);
        return;

    case AST_DATA:
        qualify_name(node->as.data.name, ns);
        if (node->as.data.addr_expr) {
            apply_namespace(node->as.data.addr_expr, ns, scope);
        }
        apply_namespace_list(node->as.data.body, node->as.data.body_count, ns, scope);
        return;

    case AST_REPEAT: apply_namespace(node->as.repeat.form, ns, scope); return;

    case AST_ADD:
    case AST_SUB:
    case AST_AND:
    case AST_OR:
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
        apply_namespace(node->as.binary.left, ns, scope);
        apply_namespace(node->as.binary.right, ns, scope);
        return;

    case AST_NEG:
    case AST_INC:
    case AST_DEC:
    case AST_NOT:
    case AST_LNOT:
    case AST_HI:
    case AST_LO:
    case AST_ADDR16: apply_namespace(node->as.unary.operand, ns, scope); return;

    case AST_ADDR:
        apply_namespace(node->as.addr.hi, ns, scope);
        apply_namespace(node->as.addr.lo, ns, scope);
        return;

    case AST_LOAD: apply_namespace(node->as.load.addr, ns, scope); return;

    case AST_STORE:
        apply_namespace(node->as.store.addr, ns, scope);
        apply_namespace(node->as.store.value, ns, scope);
        return;

    case AST_CALL:
        qualify_name(node->as.call.func, ns);
        apply_namespace_list(node->as.call.args, node->as.call.arg_count, ns, scope);
        return;
    }
}

static void get_dirname(const char* path, char* out, size_t out_size) {
    const char* slash = strrchr(path, '/');
    if (!slash) {
        strncpy(out, ".", out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }
    size_t len = (size_t)(slash - path);
    if (len >= out_size) len = out_size - 1;
    memcpy(out, path, len);
    out[len] = '\0';
}

static bool join_path(char* out, size_t out_size, const char* base, const char* rel) {
    if (!base || !*base) {
        snprintf(out, out_size, "%s", rel);
    } else {
        snprintf(out, out_size, "%s/%s", base, rel);
    }
    return out[0] != '\0';
}

static bool namespace_to_path(const char* ns, char* out, size_t out_size) {
    size_t len = strlen(ns);
    if (len + 4 >= out_size) return false;
    size_t j = 0;
    for (size_t i = 0; i < len && j + 4 < out_size; i++) {
        char c = ns[i];
        out[j++] = (c == '.') ? '/' : c;
    }
    out[j] = '\0';
    strncat(out, ".se", out_size - strlen(out) - 1);
    return true;
}

static bool is_absolute_path(const char* path) { return path[0] == '/'; }

static bool append_program_node(AstProgram* program, AstNode* node) {
    size_t max_nodes = SE_MAX_FUNCTIONS + SE_MAX_CONSTANTS;
    if (program->node_count >= max_nodes) return false;
    program->nodes[program->node_count++] = node;
    return true;
}

static bool parse_file_into_program(const char* filename, AstPool* pool, AstProgram* program) {
    size_t source_len;
    char* source = read_file(filename, &source_len);
    if (!source) return false;

    SeParser parser;
    se_parser_init(&parser, source, source_len, pool);

    if (!se_parser_parse_program(&parser, program) || se_parser_has_error(&parser)) {
        se_parser_print_error(&parser, filename);
        free(source);
        return false;
    }

    free(source);
    return true;
}

static const char* g_search_path = NULL;

static bool try_resolve_file(const char* path, char* out, size_t out_size) {
    FILE* f = fopen(path, "r");
    if (f) {
        fclose(f);
        char resolved[PATH_MAX];
        if (realpath(path, resolved)) {
            strncpy(out, resolved, out_size - 1);
            out[out_size - 1] = '\0';
            return true;
        }
        strncpy(out, path, out_size - 1);
        out[out_size - 1] = '\0';
        return true;
    }
    return false;
}

static bool parse_requires_recursive(const char* filename, AstPool* pool, AstProgram* out_program,
                                     SeModuleList* loaded) {
    char resolved[PATH_MAX];
    const char* module_key = filename;
    if (realpath(filename, resolved)) {
        module_key = resolved;
    }

    if (module_list_contains(loaded, module_key)) return true;
    if (!module_list_add(loaded, module_key)) {
        fprintf(stderr, "error: too many required modules\n");
        return false;
    }

    AstProgram program;
    if (!parse_file_into_program(filename, pool, &program)) return false;

    char namespace_name[SE_MAX_SYMBOL_LEN] = "";
    bool namespace_seen = false;
    for (size_t i = 0; i < program.node_count; i++) {
        AstNode* node = program.nodes[i];
        if (node->kind == AST_NS) {
            if (namespace_seen) {
                fprintf(stderr, "%s:%zu:%zu: error: multiple ns forms in file\n", filename,
                        node->line, node->column);
                return false;
            }
            namespace_seen = true;
            strncpy(namespace_name, node->as.symbol.name, sizeof(namespace_name) - 1);
            namespace_name[sizeof(namespace_name) - 1] = '\0';
        }
    }

    char base_dir[PATH_MAX];
    get_dirname(filename, base_dir, sizeof(base_dir));

    for (size_t i = 0; i < program.node_count; i++) {
        AstNode* node = program.nodes[i];
        if (node->kind != AST_REQUIRE) continue;

        for (size_t j = 0; j < node->as.block.expr_count; j++) {
            AstNode* req = node->as.block.exprs[j];
            char req_path[PATH_MAX];
            const char* raw_name = NULL;
            bool is_symbol = false;

            if (req->kind == AST_SYMBOL) {
                raw_name = req->as.symbol.name;
                is_symbol = true;
            } else if (req->kind == AST_STRING) {
                raw_name = req->as.symbol.name;
            } else {
                fprintf(stderr, "%s:%zu:%zu: error: require expects a symbol or string\n", filename,
                        node->line, node->column);
                return false;
            }

            if (is_symbol) {
                if (!namespace_to_path(raw_name, req_path, sizeof(req_path))) {
                    fprintf(stderr, "%s:%zu:%zu: error: require namespace too long\n", filename,
                            node->line, node->column);
                    return false;
                }
            } else {
                strncpy(req_path, raw_name, sizeof(req_path) - 1);
                req_path[sizeof(req_path) - 1] = '\0';
            }

            char full_path[PATH_MAX];
            bool found = false;

            if (is_absolute_path(req_path)) {
                strncpy(full_path, req_path, sizeof(full_path) - 1);
                full_path[sizeof(full_path) - 1] = '\0';
                found = try_resolve_file(full_path, full_path, sizeof(full_path));
            } else {
                // Try relative to current file
                if (join_path(full_path, sizeof(full_path), base_dir, req_path)) {
                    found = try_resolve_file(full_path, full_path, sizeof(full_path));
                }

                // Try search path if not found
                if (!found && g_search_path && *g_search_path) {
                    if (join_path(full_path, sizeof(full_path), g_search_path, req_path)) {
                        found = try_resolve_file(full_path, full_path, sizeof(full_path));
                    }
                }
            }

            if (!found) {
                fprintf(stderr, "%s:%zu:%zu: error: cannot find required module '%s'\n", filename,
                        node->line, node->column, req_path);
                return false;
            }

            if (!parse_requires_recursive(full_path, pool, out_program, loaded)) return false;
        }
    }

    SeScope scope = {0};
    for (size_t i = 0; i < program.node_count; i++) {
        AstNode* node = program.nodes[i];
        if (node->kind == AST_NS || node->kind == AST_REQUIRE) continue;
        apply_namespace(node, namespace_name, &scope);
        if (!append_program_node(out_program, node)) {
            fprintf(stderr, "%s:%zu:%zu: error: too many top-level forms\n", filename, node->line,
                    node->column);
            return false;
        }
    }

    return true;
}

static bool parse_with_requires(const char* filename, AstPool* pool, AstProgram* program) {
    program->node_count = 0;
    SeModuleList loaded = {0};
    return parse_requires_recursive(filename, pool, program, &loaded);
}

int main(int argc, char** argv) {
    make_and_parse_args(argc, argv);

    // Set search path (default to stdlib/se)
    if (args.search_path && *args.search_path) {
        g_search_path = args.search_path;
    } else {
        g_search_path = "stdlib/se";
    }

    static AstPool pool;
    ast_pool_reset(&pool);

    AstProgram program;
    if (!parse_with_requires(args.source_filename, &pool, &program)) {
        return EXIT_FAILURE;
    }

    // Open output file
    FILE* output = fopen(args.output_filename, "w");
    if (!output) {
        perror("could not open output file");
        return EXIT_FAILURE;
    }

    // Generate code
    SeCodegen codegen;
    se_codegen_init(&codegen, output, args.source_filename);

    if (!se_codegen_collect(&codegen, &program)) {
        se_codegen_print_error(&codegen);
        fclose(output);
        return EXIT_FAILURE;
    }

    if (!se_codegen_emit(&codegen, &program)) {
        se_codegen_print_error(&codegen);
        fclose(output);
        return EXIT_FAILURE;
    }

    fclose(output);

    return EXIT_SUCCESS;
}

static char* read_file(const char* filename, size_t* out_len) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        perror("could not open source file");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (len < 0) {
        perror("could not determine file size");
        fclose(f);
        return NULL;
    }

    char* content = malloc((size_t)len + 1);
    if (!content) {
        perror("out of memory");
        fclose(f);
        return NULL;
    }

    size_t read_len = fread(content, 1, (size_t)len, f);
    fclose(f);

    content[read_len] = '\0';
    if (out_len) *out_len = read_len;

    return content;
}
