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
#include "context.c"
#include "context.h"
#include "cpu.c"
#include "cpu.h"
#include "memory.c"
#include "memory.h"
#include "strings.c"
#include "strings.h"

#define TINY16_ASM_LINE_BUFFER_SIZE 256

int main(int argc, char** argv) {
    make_and_parse_args(argc, argv);

    Tiny16AsmContext ctx = {
        .current_section = SECTION_CODE,
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

    char buffer[TINY16_ASM_LINE_BUFFER_SIZE];

    ctx.source_file = fopen(args.source_filename, "r");
    if (ctx.source_file == NULL) {
        perror("could not open input file");
        return EXIT_FAILURE;
    }

    //
    // pass 1
    //

    ctx.source_line_no = 0;
    while (fgets(buffer, TINY16_ASM_LINE_BUFFER_SIZE, ctx.source_file) != NULL) {
        ctx.source_line_no++;

        ctx.source_line = buffer;
        tiny16_asm_str_strip_comment(ctx.source_line);
        tiny16_asm_str_trim_left(ctx.source_line);
        if (strlen(ctx.source_line) == 0)
            continue;

        tiny16_asm_section_t section = tiny16_asm_section(ctx.source_line);
        if (section != SECTION_UNKNOWN) {
            ctx.current_section = section;
            continue;
        }

        int label_length = tiny16_asm_label_length(ctx.source_line);
        if (label_length > 0) {
            if (label_length - 1 >= TINY16_ASM_MAX_LABEL_NAME_LENGTH) {
                TINY16_ASM_ABORTF(&ctx, "max label name length exceeded: %d (limit is %d)",
                                  label_length, TINY16_ASM_MAX_LABEL_NAME_LENGTH);
            }

            char tmp[TINY16_ASM_MAX_LABEL_NAME_LENGTH];
            strncpy(tmp, ctx.source_line, label_length - 1);
            if (tiny16_asm_ctx_label_addr(&ctx, tmp) != TINY16_ASM_LABEL_NOT_FOUND) {
                TINY16_ASM_ABORTF(&ctx, "duplicated label: %s", tmp);
            }

            if (ctx.label_count >= TINY16_ASM_MAX_LABELS) {
                TINY16_ASM_ABORT(&ctx, "too many labels");
            }

            switch (ctx.current_section) {
            case SECTION_CODE: ctx.labels[ctx.label_count].addr = ctx.code_pc; break;
            case SECTION_DATA: ctx.labels[ctx.label_count].addr = ctx.data_pc; break;
            default: assert(0 && "unreachable"); break;
            }

            strncpy(ctx.labels[ctx.label_count].name, ctx.source_line, label_length - 1);
            ctx.labels[ctx.label_count].name[label_length - 1] = '\0';
            ctx.label_count++;

            ctx.source_line += label_length;
            tiny16_asm_str_trim_left(ctx.source_line);
            if (strlen(ctx.source_line) == 0)
                continue;
        }

        switch (ctx.current_section) {
        case SECTION_UNKNOWN: assert(0 && "unreachable"); break;
        case SECTION_CODE: ctx.code_pc += 3; break;
        case SECTION_DATA: {
            char *rest, *mnemonic = strtok_r(ctx.source_line, " ", &rest);
            if (!mnemonic) {
                TINY16_ASM_ABORTF(&ctx, "could not parse instruction: %s", mnemonic);
            }
            ctx.source_line = rest;
            if (strncasecmp(mnemonic, "DB", 2) == 0) {
                tiny16_asm_str_trim_left(ctx.source_line);
                tiny16_asm_ctx_parse_db(&ctx);
            } else {
                TINY16_ASM_ABORTF(&ctx, "unknown instruction: %s", mnemonic);
            }

        }; break;
        }
    }

    //
    // pass 2
    //

    ctx.current_section = SECTION_CODE;
    ctx.code_pc = TINY16_MEMORY_CODE_BEGIN;
    ctx.data_pc = TINY16_MEMORY_DATA_BEGIN;
    fseek(ctx.source_file, 0L, SEEK_SET);
    ctx.source_line_no = 0;

    while (fgets(buffer, TINY16_ASM_LINE_BUFFER_SIZE, ctx.source_file) != NULL) {
        ctx.source_line_no++;

        ctx.source_line = buffer;
        tiny16_asm_str_strip_comment(ctx.source_line);
        tiny16_asm_str_trim_left(ctx.source_line);
        if (strlen(ctx.source_line) == 0)
            continue;

        tiny16_asm_section_t section = tiny16_asm_section(ctx.source_line);
        if (section != SECTION_UNKNOWN) {
            ctx.current_section = section;
            continue;
        }

        int label_length = tiny16_asm_label_length(ctx.source_line);
        if (label_length > 0) {
            ctx.source_line += label_length;
            tiny16_asm_str_trim_left(ctx.source_line);
            if (strlen(ctx.source_line) == 0)
                continue;
        }
        tiny16_asm_str_trim_right(ctx.source_line);

        switch (ctx.current_section) {
        case SECTION_UNKNOWN: assert(0 && "unreachable"); break;
        case SECTION_CODE: {
            tiny16_asm_ctx_emit_code(&ctx);
            if (ctx.current_section == SECTION_CODE) {
                ctx.code_pc += 3;
            }
        }; break;
        case SECTION_DATA: break; // ignore, already parsed in pass 1
        }
    }

    if (ctx.data_size > 0)
        tiny16_asm_ctx_emit_data(&ctx);

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
