#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char** argv) {
    make_and_parse_args(argc, argv);

    size_t source_len;
    char* source = read_file(args.source_filename, &source_len);
    if (!source) return EXIT_FAILURE;

    static AstPool pool;
    ast_pool_reset(&pool);

    SeParser parser;
    se_parser_init(&parser, source, source_len, &pool);

    AstProgram program;
    if (!se_parser_parse_program(&parser, &program)) {
        se_parser_print_error(&parser, args.source_filename);
        free(source);
        return EXIT_FAILURE;
    }

    if (se_parser_has_error(&parser)) {
        se_parser_print_error(&parser, args.source_filename);
        free(source);
        return EXIT_FAILURE;
    }

    // Open output file
    FILE* output = fopen(args.output_filename, "w");
    if (!output) {
        perror("could not open output file");
        free(source);
        return EXIT_FAILURE;
    }

    // Generate code
    SeCodegen codegen;
    se_codegen_init(&codegen, output, args.source_filename);

    if (!se_codegen_collect(&codegen, &program)) {
        se_codegen_print_error(&codegen);
        fclose(output);
        free(source);
        return EXIT_FAILURE;
    }

    if (!se_codegen_emit(&codegen, &program)) {
        se_codegen_print_error(&codegen);
        fclose(output);
        free(source);
        return EXIT_FAILURE;
    }

    fclose(output);
    free(source);

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
