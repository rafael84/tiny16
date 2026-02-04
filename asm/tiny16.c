#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.c"
#include "args.h"
#include "cpu.c"
#include "cpu.h"
#include "lexer.c"
#include "lexer.h"
#include "memory.c"
#include "memory.h"
#include "parser.c"
#include "parser.h"
#include "preprocessor.c"
#include "preprocessor.h"

static char* preprocess_source(const char* filename);

int main(int argc, char** argv) {
    make_and_parse_args(argc, argv);

    // Pass 0: Preprocess (expand macros and includes)

    char* source_content = preprocess_source(args.source_filename);
    if (source_content == NULL) {
        fprintf(stderr, "Preprocessing failed\n");
        return EXIT_FAILURE;
    }

    static Tiny16Parser parser;
    tiny16_parser_init(&parser);
    strncpy(parser.current_filename, args.source_filename, TINY16_PARSER_MAX_FILENAME - 1);
    parser.current_filename[TINY16_PARSER_MAX_FILENAME - 1] = '\0';
    parser.source_filename = parser.current_filename;
    parser.line_base = 0;
    parser.current_section = TINY16_PARSER_SECTION_CODE;
    parser.code_pc = TINY16_MEMORY_CODE_BEGIN;
    parser.data_pc = TINY16_MEMORY_DATA_BEGIN;
    parser.error = TINY16_PARSER_OK;
    parser.error_line = 0;

    parser.output_file = fopen(args.output_filename, "wb");
    if (parser.output_file == NULL) {
        perror("could not open output file");
        free(source_content);
        return EXIT_FAILURE;
    }

    // clang-format off
    uint8_t signature[16] = {
        'T', '1', '6', 0x00,                                                         // Magic
        TINY16_VERSION_MAJOR, TINY16_VERSION_MINOR,                                  // Version
        ((TINY16_MEMORY_CODE_BEGIN >> 8) & 0xFF), (TINY16_MEMORY_CODE_BEGIN & 0xFF), // Entrypoint
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                              // Reserved
    };
    // clang-format on

    parser.output_file_size = fwrite(signature, 1, sizeof signature, parser.output_file);
    if (parser.output_file_size != 16) {
        perror("write signature");
        free(source_content);
        return EXIT_FAILURE;
    }

    // Pass 1: Collect labels and parse data section

    parser.lexer = lexer_new(source_content, strlen(source_content));
    tiny16_parser_next(&parser);

    while (parser.current_token.kind != TOKEN_END && !tiny16_parser_has_error(&parser)) {
        tiny16_parser_skip_trivia(&parser);
        if (parser.current_token.kind == TOKEN_END) break;

        if (tiny16_parser_parse_file_directive(&parser) ||
            tiny16_parser_parse_line_directive(&parser)) {
            tiny16_parser_skip_to_eol(&parser);
            continue;
        }

        if (tiny16_parser_parse_const(&parser)) {
            tiny16_parser_skip_to_eol(&parser);
            continue;
        }

        if (parser.current_section == TINY16_PARSER_SECTION_DATA) {
            if (tiny16_parser_parse_org(&parser)) {
                tiny16_parser_skip_to_eol(&parser);
                continue;
            }
        }

        if (tiny16_parser_parse_section(&parser)) {
            tiny16_parser_skip_to_eol(&parser);
            continue;
        }

        if (tiny16_parser_parse_label(&parser)) {
            tiny16_parser_skip_trivia(&parser);
            if (parser.current_token.kind == TOKEN_END) break;
            continue;
        }

        uint16_t times = tiny16_parser_parse_times_prefix(&parser);

        if (parser.current_section == TINY16_PARSER_SECTION_CODE) {
            parser.code_pc += 3 * times;
            tiny16_parser_skip_to_eol(&parser);
        } else if (parser.current_section == TINY16_PARSER_SECTION_DATA) {
            if (parser.current_token.kind == TOKEN_KEYWORD) {
                for (uint16_t i = 0; i < times; ++i) {
                    Lexer saved_lexer = parser.lexer;
                    Token saved_token = parser.current_token;
                    tiny16_parser_parse_data(&parser);
                    if (i < times - 1) {
                        parser.lexer = saved_lexer;
                        parser.current_token = saved_token;
                    }
                }
            }
            tiny16_parser_skip_to_eol(&parser);
        }
    }

    if (tiny16_parser_has_error(&parser)) {
        tiny16_parser_print_error(&parser);
        free(source_content);
        fclose(parser.output_file);
        tiny16_parser_free(&parser);
        return EXIT_FAILURE;
    }

    // Pass 2: Emit code section

    parser.current_section = TINY16_PARSER_SECTION_CODE;
    parser.code_pc = TINY16_MEMORY_CODE_BEGIN;
    parser.line_base = 0;
    strncpy(parser.current_filename, args.source_filename, TINY16_PARSER_MAX_FILENAME - 1);
    parser.current_filename[TINY16_PARSER_MAX_FILENAME - 1] = '\0';
    parser.source_filename = parser.current_filename;
    parser.lexer = lexer_new(source_content, strlen(source_content));
    tiny16_parser_next(&parser);

    while (parser.current_token.kind != TOKEN_END && !tiny16_parser_has_error(&parser)) {
        tiny16_parser_skip_trivia(&parser);
        if (parser.current_token.kind == TOKEN_END) break;

        if (tiny16_parser_parse_file_directive(&parser) ||
            tiny16_parser_parse_line_directive(&parser)) {
            tiny16_parser_skip_to_eol(&parser);
            continue;
        }

        if (tiny16_parser_parse_section(&parser)) {
            tiny16_parser_skip_to_eol(&parser);
            continue;
        }

        if (tiny16_parser_skip_label(&parser)) {
            tiny16_parser_skip_trivia(&parser);
            if (parser.current_token.kind == TOKEN_END) break;
            continue;
        }

        if (parser.current_token.kind == TOKEN_SYMBOL &&
            tiny16_parser_peek(&parser, 1).kind == TOKEN_EQUALS) {
            tiny16_parser_skip_to_eol(&parser);
            continue;
        }

        uint16_t times = tiny16_parser_parse_times_prefix(&parser);

        if (parser.current_section == TINY16_PARSER_SECTION_CODE) {
            for (uint16_t i = 0; i < times; ++i) {
                Lexer saved_lexer = parser.lexer;
                Token saved_token = parser.current_token;
                tiny16_parser_emit_code(&parser);
                if (i < times - 1) {
                    parser.lexer = saved_lexer;
                    parser.current_token = saved_token;
                }
            }
            parser.code_pc += 3 * times;
        }

        tiny16_parser_skip_to_eol(&parser);
    }

    if (tiny16_parser_has_error(&parser)) {
        tiny16_parser_print_error(&parser);
        free(source_content);
        fclose(parser.output_file);
        tiny16_parser_free(&parser);
        return EXIT_FAILURE;
    }

    if (parser.data_size > 0) tiny16_parser_emit_data(&parser);

    free(source_content);
    if (fclose(parser.output_file) != 0) {
        perror("could not close output file");
        tiny16_parser_free(&parser);
        return EXIT_FAILURE;
    }
    tiny16_parser_free(&parser);

    return 0;
}

static char* preprocess_source(const char* filename) {
    static Tiny16Preprocessor pp;
    tiny16_pp_init(&pp);

    // Set search path (default to stdlib/asm)
    if (args.search_path && *args.search_path) {
        pp.search_path = args.search_path;
    } else {
        pp.search_path = "stdlib/asm";
    }

    char* preprocessed = tiny16_pp_process_file(&pp, filename);
    if (!preprocessed) {
        tiny16_pp_print_error(&pp);
        tiny16_pp_free(&pp);
        return NULL;
    }

    tiny16_pp_free(&pp);
    return preprocessed;
}
