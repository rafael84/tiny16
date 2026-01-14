#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"
#include "cpu.h"
#include "memory.h"
#include "parser.h"

#define TINY16_PARSER_ABORT(ctx, fmt)                                                              \
    do {                                                                                           \
        fprintf(stderr, "%s:%d: " fmt "\n", args.source_filename, (ctx)->source_line_no);          \
        exit(1);                                                                                   \
    } while (0)

#define TINY16_PARSER_ABORTF(ctx, fmt, ...)                                                        \
    do {                                                                                           \
        fprintf(stderr, "%s:%d: " fmt "\n", args.source_filename, (ctx)->source_line_no,           \
                __VA_ARGS__);                                                                      \
        exit(1);                                                                                   \
    } while (0)

void tiny16_parser_strip_comment(Tiny16AsmContext* ctx) {
    char* cut = strchr(ctx->source_line, ';');
    if (cut) *cut = '\0';
}

void tiny16_parser_trim_left(Tiny16AsmContext* ctx) {
    char* str = ctx->source_line;
    char* start = str;
    while (isspace((unsigned char)*start))
        start++;
    if (start != str) memmove(str, start, strlen(start) + 1);
}

void tiny16_parser_trim_right(Tiny16AsmContext* ctx) {
    char* str = ctx->source_line;
    if (*str == '\0') return;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;
    end[1] = '\0';
}

static bool tiny16_parser_is_valid_label_prefix(char* str) {
    return (str && (isalpha(str[0]) || str[0] == '_' || str[0] == '.'));
}

char* tiny16_parser_next_line(Tiny16AsmContext* ctx) {
    return fgets(ctx->line_buffer, TINY16_PARSER_LINE_BUFFER_SIZE, ctx->source_file);
}

bool tiny16_parser_parse_label(Tiny16AsmContext* ctx) {
    int label_length = tiny16_parser_label_length(ctx);
    if (label_length > 0) {
        if (label_length - 1 >= TINY16_PARSER_MAX_TOKEN_LENGTH) {
            TINY16_PARSER_ABORTF(ctx, "max label name length exceeded: %d (limit is %d)",
                                 label_length, TINY16_PARSER_MAX_TOKEN_LENGTH);
        }

        char tmp[TINY16_PARSER_MAX_TOKEN_LENGTH];
        strncpy(tmp, ctx->source_line, label_length - 1);
        tmp[label_length - 1] = '\0';

        if (tiny16_parser_label_addr(ctx, tmp) != TINY16_PARSER_LABEL_NOT_FOUND)
            TINY16_PARSER_ABORTF(ctx, "duplicated label: %s", tmp);
        if (ctx->label_count >= TINY16_PARSER_MAX_LABELS)
            TINY16_PARSER_ABORT(ctx, "too many labels");

        uint16_t addr =
            (ctx->current_section == TINY16_PARSER_SECTION_CODE) ? ctx->code_pc : ctx->data_pc;
        ctx->labels[ctx->label_count].addr = addr;
        strncpy(ctx->labels[ctx->label_count].name, tmp, TINY16_PARSER_MAX_TOKEN_LENGTH);
        ctx->label_count++;

        ctx->source_line += label_length;
        tiny16_parser_trim_left(ctx);
        if (strlen(ctx->source_line) == 0) return true;
    }
    return false;
}

int tiny16_parser_label_length(Tiny16AsmContext* ctx) {
    char* str = ctx->source_line;
    int len = strlen(str);
    int i;
    if (!(tiny16_parser_is_valid_label_prefix(str))) return 0;
    for (i = 1; i < len; ++i) {
        bool valid = isalpha(str[i]) || isdigit(str[i]) || str[i] == '.' || str[i] == '_';
        if (!valid) break;
    }
    if (i > 0 && i < len && str[i] == ':') return i + 1;
    return 0;
}

tiny16_parser_section_t tiny16_parser_section(Tiny16AsmContext* ctx) {
    char* str = ctx->source_line;
    if (strncasecmp("section", str, 7) == 0) {
        str += 7;
        while (*str && isspace((unsigned char)*str))
            str++;
        if (*str != '\0') {
            if (strncmp(str, ".code", 5) == 0) return TINY16_PARSER_SECTION_CODE;
            if (strncmp(str, ".data", 5) == 0) return TINY16_PARSER_SECTION_DATA;
        }
    }
    return TINY16_PARSER_SECTION_UNKNOWN;
}

uint16_t tiny16_parser_label_addr(Tiny16AsmContext* ctx, char* name) {
    for (int i = 0; i < ctx->label_count; ++i) {
        if (strcmp(ctx->labels[i].name, name) == 0) return ctx->labels[i].addr;
    }
    return TINY16_PARSER_LABEL_NOT_FOUND;
}

static bool tiny16_parser_is_sep(char c) { return c == ',' || c == ' ' || c == '\t'; }

void tiny16_parser_skip_sep(Tiny16AsmContext* ctx) {
    while (tiny16_parser_is_sep(*ctx->source_line))
        ctx->source_line++;
}

char* tiny16_parser_read_token(Tiny16AsmContext* ctx) {
    size_t len = 0;
    size_t max_len = TINY16_PARSER_MAX_TOKEN_LENGTH;
    while (*ctx->source_line && !tiny16_parser_is_sep(*ctx->source_line) && len < max_len - 1) {
        ctx->token_buffer[len++] = *ctx->source_line++;
    }
    ctx->token_buffer[len] = '\0';
    return ctx->token_buffer;
}

void tiny16_parser_expect_end(Tiny16AsmContext* ctx) {
    tiny16_parser_skip_sep(ctx);
    if (*ctx->source_line != '\0') TINY16_PARSER_ABORT(ctx, "too many operands");
}

Tiny16OpCode tiny16_parser_parse_mnemonic(Tiny16AsmContext* ctx) {
    tiny16_parser_skip_sep(ctx);
    char* mnemonic = tiny16_parser_read_token(ctx);
    if (*mnemonic == '\0') TINY16_PARSER_ABORT(ctx, "could not parse mnemonic");

    Tiny16OpCode opcode = tiny16_opcode_from_mnemonic(mnemonic);
    if (opcode == TINY16_OPCODE_UNKNOWN)
        TINY16_PARSER_ABORTF(ctx, "unknown mnemonic: %s", mnemonic);

    return opcode;
}

uint8_t tiny16_parser_parse_reg(Tiny16AsmContext* ctx) {
    tiny16_parser_skip_sep(ctx);
    char c = toupper(*ctx->source_line);
    if (c != 'R') TINY16_PARSER_ABORTF(ctx, "expected register, found `%c`", *ctx->source_line);
    ctx->source_line++;
    char d = *ctx->source_line;
    if (d < '0' || d > '7') TINY16_PARSER_ABORTF(ctx, "invalid register number: `%c`", d);
    ctx->source_line++;
    return (uint8_t)(d - '0');
}

uint16_t tiny16_parser_parse_imm(Tiny16AsmContext* ctx) {
    tiny16_parser_skip_sep(ctx);
    if (*ctx->source_line == '\0') TINY16_PARSER_ABORT(ctx, "immediate expected");
    if (isalpha(*ctx->source_line) || *ctx->source_line == '_') {
        char* label = tiny16_parser_read_token(ctx);
        uint16_t addr = tiny16_parser_label_addr(ctx, label);
        if (addr == TINY16_PARSER_LABEL_NOT_FOUND)
            TINY16_PARSER_ABORTF(ctx, "label %s not found", label);
        return addr;
    }
    long val = tiny16_parser_parse_number(ctx);
    if (val < 0 || val > UINT16_MAX) TINY16_PARSER_ABORTF(ctx, "immediate out of bounds: %ld", val);
    return (uint16_t)val;
}

uint8_t tiny16_parser_parse_imm8(Tiny16AsmContext* ctx) {
    uint16_t imm = tiny16_parser_parse_imm(ctx);
    if (imm > UINT8_MAX) TINY16_PARSER_ABORTF(ctx, "immediate out of bounds: %" PRIu16, imm);
    return (uint8_t)imm;
}

void tiny16_parser_eat_space(Tiny16AsmContext* ctx) {
    while (*ctx->source_line && isspace(*ctx->source_line))
        ctx->source_line++;
}

void tiny16_parser_eat_char(Tiny16AsmContext* ctx, char c) {
    if (*ctx->source_line != c) TINY16_PARSER_ABORTF(ctx, "expected `%c`", c);
    ctx->source_line++;
}

long tiny16_parser_parse_number(Tiny16AsmContext* ctx) {
    if (!isdigit(*ctx->source_line))
        TINY16_PARSER_ABORTF(ctx, "expected number, found `%s`", ctx->source_line);

    long value;
    char* end;
    int base = 10;

    if (*ctx->source_line == '0') {
        char prefix = ctx->source_line[1];
        if (prefix == 'x' || prefix == 'X') {
            base = 16;
        } else if (prefix == 'b' || prefix == 'B') {
            ctx->source_line += 2; // skip 0b prefix since strtol doesn't support it
            base = 2;
        }
    }

    value = strtol(ctx->source_line, &end, base);
    ctx->source_line = end;
    return value;
}

Tiny16Addr tiny16_parser_parse_addr(Tiny16AsmContext* ctx) {
    Tiny16Addr addr = {0};

    // Parse destination/source register: R0, [...]
    addr.reg = tiny16_parser_parse_reg(ctx);
    tiny16_parser_skip_sep(ctx);

    tiny16_parser_eat_char(ctx, '[');
    tiny16_parser_eat_space(ctx);

    uint8_t rh = tiny16_parser_parse_reg(ctx);
    if (rh % 2 != 0) TINY16_PARSER_ABORTF(ctx, "expected even register, found R%d", rh);

    tiny16_parser_eat_space(ctx);
    tiny16_parser_eat_char(ctx, ':');
    tiny16_parser_eat_space(ctx);

    uint8_t rl = tiny16_parser_parse_reg(ctx);
    if (rl != rh + 1) TINY16_PARSER_ABORTF(ctx, "expected R%d, found R%d", rh + 1, rl);

    addr.pair = rh / 2;
    addr.mode = TINY16_ADDR_MODE_BASE;
    addr.offset = 0;

    tiny16_parser_eat_space(ctx);

    // Check for offset inside brackets: [R6:R7 + 10]
    if (*ctx->source_line == '+' || *ctx->source_line == '-') {
        char sign = *ctx->source_line;
        ctx->source_line++;
        tiny16_parser_eat_space(ctx);
        long val = tiny16_parser_parse_number(ctx);
        if (sign == '-') val = -val;
        if (val < 0 || val > UINT8_MAX)
            TINY16_PARSER_ABORTF(ctx, "offset out of bounds: %ld (must be 0-255)", val);
        addr.mode = TINY16_ADDR_MODE_OFFSET;
        addr.offset = (uint8_t)val;
        tiny16_parser_eat_space(ctx);
    }

    tiny16_parser_eat_char(ctx, ']');

    // Check for post-increment/decrement suffix: [R6:R7]+ or [R6:R7]-
    if (addr.mode == TINY16_ADDR_MODE_BASE) {
        if (*ctx->source_line == '+') {
            addr.mode = TINY16_ADDR_MODE_INC;
            ctx->source_line++;
        } else if (*ctx->source_line == '-') {
            addr.mode = TINY16_ADDR_MODE_DEC;
            ctx->source_line++;
        }
    }

    tiny16_parser_eat_space(ctx);
    return addr;
}

void tiny16_parser_emit_code(Tiny16AsmContext* ctx) {
    const uint16_t max_program_size = TINY16_MEMORY_CODE_END - TINY16_MEMORY_CODE_BEGIN;
    if ((ctx->output_file_size + 3) > max_program_size)
        TINY16_PARSER_ABORTF(ctx, "max program size is %d bytes", max_program_size);

    Tiny16OpCode opcode = tiny16_parser_parse_mnemonic(ctx);

    uint8_t bytes[3];
    bytes[0] = opcode;

    switch (opcode) {
    case TINY16_OPCODE_LOADI:
        bytes[1] = tiny16_parser_parse_reg(ctx);
        bytes[2] = tiny16_parser_parse_imm8(ctx);
        break;

    case TINY16_OPCODE_LOAD:
    case TINY16_OPCODE_STORE: {
        Tiny16Addr addr = tiny16_parser_parse_addr(ctx);
        bytes[1] = TINY16_ADDR_BYTE1(addr.reg, addr.mode, addr.pair);
        bytes[2] = addr.offset;
    } break;

    case TINY16_OPCODE_MOV:
    case TINY16_OPCODE_ADD:
    case TINY16_OPCODE_SUB:
    case TINY16_OPCODE_AND:
    case TINY16_OPCODE_OR:
    case TINY16_OPCODE_XOR:
    case TINY16_OPCODE_CMP:
    case TINY16_OPCODE_ADC:
    case TINY16_OPCODE_SBC:
        bytes[1] = tiny16_parser_parse_reg(ctx);
        bytes[2] = tiny16_parser_parse_reg(ctx);
        break;

    case TINY16_OPCODE_INC:
    case TINY16_OPCODE_DEC:
    case TINY16_OPCODE_SHL:
    case TINY16_OPCODE_SHR:
    case TINY16_OPCODE_PUSH:
    case TINY16_OPCODE_POP:
        bytes[1] = tiny16_parser_parse_reg(ctx);
        bytes[2] = 0;
        break;

    case TINY16_OPCODE_JMP:
    case TINY16_OPCODE_JZ:
    case TINY16_OPCODE_JNZ:
    case TINY16_OPCODE_JC:
    case TINY16_OPCODE_JNC:
    case TINY16_OPCODE_CALL: {
        uint16_t addr = tiny16_parser_parse_imm(ctx);
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

    tiny16_parser_expect_end(ctx);

    size_t n = fwrite(bytes, 1, 3, ctx->output_file);
    if (n != 3) {
        perror("write code");
        exit(1);
    }

    ctx->output_file_size += 3;
}

static char tiny16_parser_parse_escape(char c) {
    switch (c) {
    case 'n':
        return '\n';
    case 't':
        return '\t';
    case 'r':
        return '\r';
    case '\\':
        return '\\';
    case '"':
        return '"';
    case '0':
        return '\0';
    default:
        return c;
    }
}

void tiny16_parser_parse_db(Tiny16AsmContext* ctx) {
    while (*ctx->source_line) {
        tiny16_parser_skip_sep(ctx);
        if (!*ctx->source_line) break;

        // parse quoted string
        if (*ctx->source_line == '"') {
            ctx->source_line++; // skip opening quote
            while (*ctx->source_line && *ctx->source_line != '"') {
                if (*ctx->source_line == '\\' && ctx->source_line[1]) {
                    ctx->source_line++;
                    ctx->data_buffer[ctx->data_size++] =
                        tiny16_parser_parse_escape(*ctx->source_line++);
                } else {
                    ctx->data_buffer[ctx->data_size++] = *ctx->source_line++;
                }
            }
            if (*ctx->source_line == '"') ctx->source_line++; // skip closing quote
        }
        // parse number
        else if (isdigit(*ctx->source_line)) {
            long value = tiny16_parser_parse_number(ctx);
            ctx->data_buffer[ctx->data_size++] = (uint8_t)(value & 0xFF);
        }
        // skip unknown characters
        else {
            ctx->source_line++;
        }
    }
}

int tiny16_parser_parse_times_prefix(Tiny16AsmContext* ctx) {
    if (strncasecmp(ctx->source_line, "TIMES", 5) != 0 || !isspace(ctx->source_line[5])) {
        return 1; // no times prefix, execute once
    }
    ctx->source_line += 5;
    tiny16_parser_eat_space(ctx);

    long count = tiny16_parser_parse_number(ctx);
    if (count < 0 || count > UINT16_MAX) TINY16_PARSER_ABORT(ctx, "TIMES: invalid count");

    tiny16_parser_eat_space(ctx);
    return (int)count;
}

void tiny16_parser_emit_data(Tiny16AsmContext* ctx) {
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

bool tiny16_parser_preprocess_line(Tiny16AsmContext* ctx) {
    ctx->source_line = ctx->line_buffer;
    tiny16_parser_strip_comment(ctx);
    tiny16_parser_trim_left(ctx);
    return strlen(ctx->source_line) > 0;
}

bool tiny16_parser_parse_section(Tiny16AsmContext* ctx) {
    tiny16_parser_section_t section = tiny16_parser_section(ctx);
    if (section != TINY16_PARSER_SECTION_UNKNOWN) {
        ctx->current_section = section;
        return true;
    }
    return false;
}

bool tiny16_parser_skip_label(Tiny16AsmContext* ctx) {
    int label_length = tiny16_parser_label_length(ctx);
    if (label_length > 0) {
        ctx->source_line += label_length;
        tiny16_parser_trim_left(ctx);
    }
    return strlen(ctx->source_line) > 0;
}

void tiny16_parser_times_do(Tiny16AsmContext* ctx, int times, tiny16_parser_callback_fn callback) {
    char saved_line[TINY16_PARSER_LINE_BUFFER_SIZE];
    strncpy(saved_line, ctx->source_line, sizeof(saved_line) - 1);
    saved_line[sizeof(saved_line) - 1] = '\0';

    for (int i = 0; i < times; ++i) {
        char temp[TINY16_PARSER_LINE_BUFFER_SIZE];
        strncpy(temp, saved_line, sizeof(temp) - 1);
        temp[sizeof(temp) - 1] = '\0';
        ctx->source_line = temp;
        callback(ctx);
    }
}

void tiny16_parser_parse_data(Tiny16AsmContext* ctx) {
    tiny16_parser_trim_right(ctx);
    tiny16_parser_skip_sep(ctx);

    char* directive = tiny16_parser_read_token(ctx);
    if (*directive == '\0') TINY16_PARSER_ABORT(ctx, "could not parse directive");

    if (strncasecmp(directive, "DB", 2) == 0)
        tiny16_parser_parse_db(ctx);
    else
        TINY16_PARSER_ABORTF(ctx, "unknown directive: %s", directive);
}
