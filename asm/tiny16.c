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
#include "memory.c"
#include "memory.h"
#include "parser.c"
#include "parser.h"

#include "lexer.c"
#include "lexer.h"

static char* read_entire_file(const char* filename);

int main(int argc, char** argv) {
    make_and_parse_args(argc, argv);

    char* source_content = read_entire_file(args.source_filename);
    if (source_content == NULL) {
        fprintf(stderr, "Failed to read source file: %s\n", args.source_filename);
        return EXIT_FAILURE;
    }

    Tiny16Parser parser = {
        .source_filename = args.source_filename,
        .current_section = TINY16_PARSER_SECTION_CODE,
        .code_pc = TINY16_MEMORY_CODE_BEGIN,
        .data_pc = TINY16_MEMORY_DATA_BEGIN,
        .error = TINY16_PARSER_OK,
        .error_line = 0,
    };

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

    //
    // Pass 1: Collect labels and parse data section
    //

    parser.lexer = lexer_new(source_content, strlen(source_content));
    tiny16_parser_next(&parser);

    while (parser.current_token.kind != TOKEN_END && !tiny16_parser_has_error(&parser)) {
        tiny16_parser_skip_trivia(&parser);
        if (parser.current_token.kind == TOKEN_END) break;

        if (tiny16_parser_parse_section(&parser)) {
            tiny16_parser_skip_to_eol(&parser);
            continue;
        }

        if (tiny16_parser_parse_label(&parser)) {
            tiny16_parser_skip_trivia(&parser);
            if (parser.current_token.kind == TOKEN_END) break;
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
        return EXIT_FAILURE;
    }

    //
    // Pass 2: Emit code section
    //

    parser.current_section = TINY16_PARSER_SECTION_CODE;
    parser.code_pc = TINY16_MEMORY_CODE_BEGIN;
    parser.lexer = lexer_new(source_content, strlen(source_content));
    tiny16_parser_next(&parser);

    while (parser.current_token.kind != TOKEN_END && !tiny16_parser_has_error(&parser)) {
        tiny16_parser_skip_trivia(&parser);
        if (parser.current_token.kind == TOKEN_END) break;

        if (tiny16_parser_parse_section(&parser)) {
            tiny16_parser_skip_to_eol(&parser);
            continue;
        }

        if (tiny16_parser_skip_label(&parser)) {
            tiny16_parser_skip_trivia(&parser);
            if (parser.current_token.kind == TOKEN_END) break;
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
        return EXIT_FAILURE;
    }

    if (parser.data_size > 0) tiny16_parser_emit_data(&parser);

    free(source_content);
    fclose(parser.output_file);
    if (errno) {
        perror("could not close output file");
        return EXIT_FAILURE;
    }

    return 0;
}

static char* read_entire_file(const char* filename) {
    // Important: open in binary mode so `ftell()` matches `fread()` on Windows.
    // In text mode, newline translation (\r\n -> \n) can make `ftell()` and `fread()` disagree,
    // causing spurious short reads.
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Could not open file");
        return NULL;
    }
    if (fseek(file, 0L, SEEK_END) != 0) {
        perror("Could not seek to end of file");
        fclose(file);
        return NULL;
    }
    long file_size = ftell(file);
    if (file_size == -1) {
        perror("Could not get file size");
        fclose(file);
        return NULL;
    }
    if (file_size < 0 || (unsigned long)file_size > (size_t)-1 - 1) {
        fprintf(stderr, "Could not read file content: file too large\n");
        fclose(file);
        return NULL;
    }
    if (fseek(file, 0L, SEEK_SET) != 0) {
        perror("Could not seek to beginning of file");
        fclose(file);
        return NULL;
    }
    char* buffer = (char*)malloc((size_t)file_size + 1);
    if (buffer == NULL) {
        perror("Could not allocate memory");
        fclose(file);
        return NULL;
    }
    errno = 0;
    size_t bytes_read = fread(buffer, 1, (size_t)file_size, file);
    if (bytes_read != (size_t)file_size) {
        perror("Could not read file content");
        free(buffer);
        fclose(file);
        return NULL;
    }
    buffer[file_size] = '\0';
    fclose(file);
    return buffer;
}
