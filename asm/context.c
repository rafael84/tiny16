#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "args.h"
#include "context.h"
#include "memory.h"
#include "strings.h"

#define TINY16_ASM_ABORT(ctx, fmt)                                                                 \
    do {                                                                                           \
        fprintf(stderr, "%s:%d: " fmt "\n", args.source_filename, (ctx)->source_line_no);          \
        exit(1);                                                                                   \
    } while (0)

#define TINY16_ASM_ABORTF(ctx, fmt, ...)                                                           \
    do {                                                                                           \
        fprintf(stderr, "%s:%d: " fmt "\n", args.source_filename, (ctx)->source_line_no,           \
                __VA_ARGS__);                                                                      \
        exit(1);                                                                                   \
    } while (0)

uint16_t tiny16_asm_ctx_label_addr(Tiny16AsmContext* ctx, char* name) {
    for (int i = 0; i < ctx->label_count; ++i) {
        if (strcmp(ctx->labels[i].name, name) == 0) return ctx->labels[i].addr;
    }
    return TINY16_ASM_LABEL_NOT_FOUND;
}

static char* token_separator = ", \t";

Tiny16OpCode tiny16_asm_ctx_parse_mnemonic(Tiny16AsmContext* ctx, char* str, char** saveptr) {
    char* mnemonic = strtok_r(str, token_separator, saveptr);
    if (!mnemonic) TINY16_ASM_ABORTF(ctx, "could not parse mnemonic: %s", mnemonic);

    Tiny16OpCode opcode = tiny16_opcode_from_mnemonic(mnemonic);
    if (opcode == TINY16_OPCODE_UNKNOWN) TINY16_ASM_ABORTF(ctx, "unknown mnemonic: %s", mnemonic);

    return opcode;
}

uint8_t tiny16_asm_ctx_parse_reg(Tiny16AsmContext* ctx, char* str, char** saveptr) {
    char* reg = strtok_r(str, token_separator, saveptr);
    if (!reg) TINY16_ASM_ABORTF(ctx, "could not parse register: %s", reg);
    if (strlen(reg) != 2) TINY16_ASM_ABORTF(ctx, "invalid register: %s", reg);
    if (reg[0] != 'R' || (reg[1] < '0' || reg[1] > '7'))
        TINY16_ASM_ABORTF(ctx, "register not found, %s", reg);

    return (uint8_t)(reg[1] - '0');
}

uint16_t tiny16_asm_ctx_parse_imm(Tiny16AsmContext* ctx, char* str, char** saveptr) {
    char* imm = strtok_r(str, token_separator, saveptr);
    if (!imm) TINY16_ASM_ABORTF(ctx, "immediate expected, found: %s", imm);
    if (isalpha(*imm)) {
        uint16_t addr = tiny16_asm_ctx_label_addr(ctx, imm);
        if (addr == TINY16_ASM_LABEL_NOT_FOUND) TINY16_ASM_ABORTF(ctx, "label %s not found", imm);
        return addr;
    }
    long val = tiny16_asm_str_to_long(imm);
    if (errno) TINY16_ASM_ABORTF(ctx, "invalid immediate: %s", imm);
    if (val < 0 || val > UINT16_MAX) TINY16_ASM_ABORTF(ctx, "immediate out of bounds: %ld", val);
    return (uint16_t)val;
}

uint8_t tiny16_asm_ctx_parse_imm8(Tiny16AsmContext* ctx, char* str, char** saveptr) {
    uint16_t imm = tiny16_asm_ctx_parse_imm(ctx, str, saveptr);
    if (imm > UINT8_MAX) TINY16_ASM_ABORTF(ctx, "immediate out of bounds: %" PRIu16, imm);
    return (uint8_t)imm;
}

void tiny16_asm_ctx_emit_code(Tiny16AsmContext* ctx) {
    if ((ctx->output_file_size + 3) > TINY16_MEMORY_CODE_END)
        TINY16_ASM_ABORT(ctx, "max program size is 64KB"); // TODO review this

    char* saveptr;
    Tiny16OpCode opcode = tiny16_asm_ctx_parse_mnemonic(ctx, ctx->source_line, &saveptr);

    uint8_t bytes[3];
    bytes[0] = opcode;

    switch (opcode) {
    case TINY16_OPCODE_LOADI:
        bytes[1] = tiny16_asm_ctx_parse_reg(ctx, NULL, &saveptr);
        bytes[2] = tiny16_asm_ctx_parse_imm8(ctx, NULL, &saveptr);
        break;

    case TINY16_OPCODE_MOV:
    case TINY16_OPCODE_ADD:
    case TINY16_OPCODE_SUB:
    case TINY16_OPCODE_AND:
    case TINY16_OPCODE_OR:
    case TINY16_OPCODE_XOR:
        bytes[1] = tiny16_asm_ctx_parse_reg(ctx, NULL, &saveptr);
        bytes[2] = tiny16_asm_ctx_parse_reg(ctx, NULL, &saveptr);
        break;

    case TINY16_OPCODE_LOAD:
    case TINY16_OPCODE_STORE:
    case TINY16_OPCODE_INC:
    case TINY16_OPCODE_DEC:
    case TINY16_OPCODE_SHL:
    case TINY16_OPCODE_SHR:
    case TINY16_OPCODE_PUSH:
    case TINY16_OPCODE_POP:
        bytes[1] = tiny16_asm_ctx_parse_reg(ctx, NULL, &saveptr);
        bytes[2] = 0;
        break;

    case TINY16_OPCODE_JMP:
    case TINY16_OPCODE_JZ:
    case TINY16_OPCODE_JNZ:
    case TINY16_OPCODE_JC:
    case TINY16_OPCODE_JNC:
    case TINY16_OPCODE_CALL: {
        uint16_t addr = tiny16_asm_ctx_parse_imm(ctx, NULL, &saveptr);
        bytes[1] = (addr >> 8) & 0xFF;
        bytes[2] = addr & 0xFF;
    }; break;

    case TINY16_OPCODE_RET:
    case TINY16_OPCODE_HALT:
        bytes[1] = 0;
        bytes[2] = 0;
        break;

    case TINY16_OPCODE_UNKNOWN:
        return;
    }

    if (strtok_r(NULL, token_separator, &saveptr) != NULL)
        TINY16_ASM_ABORT(ctx, "too many operands");

    size_t n = fwrite(bytes, 1, 3, ctx->output_file);
    if (n != 3) {
        perror("write code");
        exit(1);
    }

    ctx->output_file_size += 3;
}

void tiny16_asm_ctx_parse_db(Tiny16AsmContext* ctx) {
    while (*ctx->source_line) {
        // skip whitespace and commas
        while (*ctx->source_line && (isspace(*ctx->source_line) || *ctx->source_line == ',')) {
            ctx->source_line++;
        }
        if (!*ctx->source_line) break;

        // parse quoted string
        if (*ctx->source_line == '"') {
            ctx->source_line++; // skip opening quote
            while (*ctx->source_line && *ctx->source_line != '"') {
                if (*ctx->source_line == '\\') {
                    ctx->source_line++;
                    if (*ctx->source_line) {
                        // Process escape sequences
                        char escaped;
                        switch (*ctx->source_line) {
                        case 'n':
                            escaped = '\n';
                            break;
                        case 't':
                            escaped = '\t';
                            break;
                        case 'r':
                            escaped = '\r';
                            break;
                        case '\\':
                            escaped = '\\';
                            break;
                        case '"':
                            escaped = '"';
                            break;
                        case '0':
                            escaped = '\0';
                            break;
                        default:
                            escaped = *ctx->source_line;
                            break;
                        }
                        ctx->data_buffer[ctx->data_size++] = escaped;
                        ctx->source_line++;
                    }
                } else {
                    ctx->data_buffer[ctx->data_size++] = *ctx->source_line;
                    ctx->source_line++;
                }
            }
            if (*ctx->source_line == '"') {
                ctx->source_line++; // skip closing quote
            }
        }
        // parse number (decimal or hex)
        else if (isdigit(*ctx->source_line) ||
                 (*ctx->source_line == '0' &&
                  (ctx->source_line[1] == 'x' || ctx->source_line[1] == 'X'))) {
            char* end;
            long value;

            // check for hex prefix
            if (*ctx->source_line == '0' &&
                (ctx->source_line[1] == 'x' || ctx->source_line[1] == 'X')) {
                value = strtol(ctx->source_line, &end, 16);
            } else {
                value = strtol(ctx->source_line, &end, 10);
            }

            ctx->data_buffer[ctx->data_size++] = (uint8_t)(value & 0xFF);
            ctx->source_line = end;
        }
        // skip unknown characters (or could be an error)
        else {
            ctx->source_line++;
        }
    }
}

void tiny16_asm_ctx_emit_data(Tiny16AsmContext* ctx) {
    size_t n = TINY16_MEMORY_CODE_END - ctx->code_pc + 1;
    for (size_t i = 0; i < n; ++i)
        fputc('\0', ctx->output_file);
    ctx->output_file_size += n;

    n = fwrite(ctx->data_buffer, 1, ctx->data_size, ctx->output_file);
    if (n != ctx->data_size) {
        perror("write data");
        exit(1);
    }
}
