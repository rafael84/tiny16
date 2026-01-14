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

int main(int argc, char** argv) {
    make_and_parse_args(argc, argv);

    Tiny16AsmContext ctx = {
        .current_section = TINY16_PARSER_SECTION_CODE,
        .code_pc = TINY16_MEMORY_CODE_BEGIN,
        .data_pc = TINY16_MEMORY_DATA_BEGIN,
    };

    ctx.output_file = fopen(args.output_filename, "wb");
    if (ctx.output_file == NULL) {
        perror("could not open output file");
        return EXIT_FAILURE;
    }

    // clang-format off
    uint8_t signature[16] = {
        'T', '1', '6', 0x00,                                                         // Magic
        TINY16_VERSION_MAJOR, TINY16_VERSION_MINOR,                                  // Version (big-endian)
        ((TINY16_MEMORY_CODE_BEGIN >> 8) & 0xFF), (TINY16_MEMORY_CODE_BEGIN & 0xFF), // Entrypoint addr (big-endian)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                              // Reserved
    };
    // clang-format on

    ctx.output_file_size = fwrite(signature, 1, sizeof signature, ctx.output_file);
    if (ctx.output_file_size != 16) {
        perror("write signature");
        return EXIT_FAILURE;
    }

    char buffer[TINY16_PARSER_LINE_BUFFER_SIZE];

    ctx.source_file = fopen(args.source_filename, "r");
    if (ctx.source_file == NULL) {
        perror("could not open input file");
        return EXIT_FAILURE;
    }

    //
    // Pass 1: Collect labels and parse data section
    //

    ctx.source_line_no = 0;
    while (fgets(buffer, TINY16_PARSER_LINE_BUFFER_SIZE, ctx.source_file) != NULL) {
        ctx.source_line_no++;

        if (!tiny16_parser_preprocess_line(&ctx, buffer)) continue;
        if (tiny16_parser_parse_section(&ctx)) continue;

        int label_length = tiny16_parser_label_length(&ctx);
        if (label_length > 0) {
            if (label_length - 1 >= TINY16_PARSER_MAX_LABEL_NAME_LENGTH) {
                TINY16_PARSER_ABORTF(&ctx, "max label name length exceeded: %d (limit is %d)",
                                     label_length, TINY16_PARSER_MAX_LABEL_NAME_LENGTH);
            }

            char tmp[TINY16_PARSER_MAX_LABEL_NAME_LENGTH];
            strncpy(tmp, ctx.source_line, label_length - 1);
            tmp[label_length - 1] = '\0';

            if (tiny16_parser_label_addr(&ctx, tmp) != TINY16_PARSER_LABEL_NOT_FOUND)
                TINY16_PARSER_ABORTF(&ctx, "duplicated label: %s", tmp);
            if (ctx.label_count >= TINY16_PARSER_MAX_LABELS)
                TINY16_PARSER_ABORT(&ctx, "too many labels");

            uint16_t addr =
                (ctx.current_section == TINY16_PARSER_SECTION_CODE) ? ctx.code_pc : ctx.data_pc;
            ctx.labels[ctx.label_count].addr = addr;
            strncpy(ctx.labels[ctx.label_count].name, tmp, TINY16_PARSER_MAX_LABEL_NAME_LENGTH);
            ctx.label_count++;

            ctx.source_line += label_length;
            tiny16_parser_trim_left(&ctx);
            if (strlen(ctx.source_line) == 0) continue;
        }

        int times = tiny16_parser_parse_times_prefix(&ctx);

        switch (ctx.current_section) {
        case TINY16_PARSER_SECTION_CODE:
            ctx.code_pc += 3 * times;
            break;
        case TINY16_PARSER_SECTION_DATA:
            tiny16_parser_times_do(&ctx, times, tiny16_parser_parse_data);
            break;
        default:
            assert(0 && "unreachable");
        }
    }

    //
    // Pass 2: Emit code section
    //

    ctx.current_section = TINY16_PARSER_SECTION_CODE;
    ctx.code_pc = TINY16_MEMORY_CODE_BEGIN;
    ctx.data_pc = TINY16_MEMORY_DATA_BEGIN;
    fseek(ctx.source_file, 0L, SEEK_SET);
    ctx.source_line_no = 0;

    while (fgets(buffer, TINY16_PARSER_LINE_BUFFER_SIZE, ctx.source_file) != NULL) {
        ctx.source_line_no++;

        if (!tiny16_parser_preprocess_line(&ctx, buffer)) continue;
        if (tiny16_parser_parse_section(&ctx)) continue;
        if (!tiny16_parser_skip_label(&ctx)) continue;

        tiny16_parser_trim_right(&ctx);

        if (ctx.current_section == TINY16_PARSER_SECTION_CODE) {
            int times = tiny16_parser_parse_times_prefix(&ctx);
            tiny16_parser_times_do(&ctx, times, tiny16_parser_emit_code);
            ctx.code_pc += 3 * times;
        }
        // DATA section already parsed in pass 1
    }

    if (ctx.data_size > 0) tiny16_parser_emit_data(&ctx);

    fclose(ctx.source_file);
    if (errno) {
        perror("could not close input file");
        return EXIT_FAILURE;
    }

    fclose(ctx.output_file);
    if (errno) {
        perror("could not close output file");
        return EXIT_FAILURE;
    }

    return 0;
}
