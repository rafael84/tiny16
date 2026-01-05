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

#include "cpu.c"
#include "cpu.h"
#include "memory.h"

#define TINY16_ASM_ABORT(fmt)                                                                      \
    do {                                                                                           \
        fprintf(stderr, "%s:%d: " fmt "\n", filename, line_no);                                    \
        exit(1);                                                                                   \
    } while (0)

#define TINY16_ASM_ABORTF(fmt, ...)                                                                \
    do {                                                                                           \
        fprintf(stderr, "%s:%d: " fmt "\n", filename, line_no, __VA_ARGS__);                       \
        exit(1);                                                                                   \
    } while (0)

char* args_shift(int* argc, char*** argv);

void strip_comment(char* str);
void str_trim_left(char* str);
void str_trim_right(char* str);
void str_trim(char* str);
void str_to_upper(char* str);
long str_to_long(const char* str);

#define LINE_BUFFER_SIZE 256

#define MAX_LABEL_NAME_LENGTH 256
#define MAX_LABELS 4096

typedef struct {
    char name[MAX_LABEL_NAME_LENGTH];
    uint16_t addr;
} Label;

typedef struct {
    Label labels[MAX_LABELS];
    int label_count;
    FILE* out;
    size_t out_size;
} AsmContext;

#define LABEL_NOT_FOUND 0xFFFF
uint16_t label_addr(AsmContext* ctx, char* name);

Tiny16OpCode expect_mnemonic(char* str, char** saveptr, char* filename, int line_no);
uint8_t expect_reg(char* str, char** saveptr, char* filename, int line_no);
uint16_t expect_imm(AsmContext* ctx, char* str, char** saveptr, char* filename, int line_no);
uint8_t expect_imm8(AsmContext* ctx, char* str, char** saveptr, char* filename, int line_no);

void output_instruction(AsmContext* ctx, char* filename, int line_no, char* line);

bool is_valid_label_prefix(char* str);
int label_length(char* str);

int main(int argc, char** argv) {
    char* program = args_shift(&argc, &argv);

    if (argc < 2) {
        fprintf(stderr, "Usage:\n\t%s <file.asm>... <output.tiny16>\n", program);
    }

    AsmContext ctx = {0};
    ctx.out = fopen(argv[argc - 1], "wb");
    if (ctx.out == NULL) {
        perror("could not open output file");
        return EXIT_FAILURE;
    }
    argc--;

    // clang-format off
    uint8_t signature[16] = {
        'T', '1', '6', 0x00,                                                         // Magic
        TINY16_VERSION_MAJOR, TINY16_VERSION_MINOR,                                  // Version (big-endian)
        ((TINY16_MEMORY_CODE_BEGIN >> 8) | 0xFF), (TINY16_MEMORY_CODE_BEGIN | 0xFF), // Entrypoint addr (big-endian)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                              // Reserved
    };
    // clang-format on

    ctx.out_size = fwrite(signature, 1, sizeof signature, ctx.out);
    if (ctx.out_size != 16) {
        perror("write signature");
        exit(1);
    }

    FILE* file;
    char* filename;
    char buffer[LINE_BUFFER_SIZE];

    int pc = 0;

    for (int i = 0; i < argc; ++i) {
        filename = args_shift(&argc, &argv);
        file = fopen(filename, "r");
        if (file == NULL) {
            perror("could not open input file");
            return EXIT_FAILURE;
        }

        //
        // pass 1
        //

        int line_no = 0;
        while (fgets(buffer, LINE_BUFFER_SIZE, file) != NULL) {
            line_no++;

            char* line_buffer = buffer;
            strip_comment(line_buffer);
            str_trim_left(line_buffer);
            if (strlen(line_buffer) == 0)
                continue;
            int lbl_len = label_length(line_buffer);
            if (lbl_len > 0) {
                if (lbl_len - 1 >= MAX_LABEL_NAME_LENGTH) {
                    TINY16_ASM_ABORTF("max label name length exceeded: %d (limit is %d)", lbl_len,
                                      MAX_LABEL_NAME_LENGTH);
                }

                char tmp[MAX_LABEL_NAME_LENGTH];
                strncpy(tmp, line_buffer, lbl_len - 1);
                if (label_addr(&ctx, tmp) != LABEL_NOT_FOUND) {
                    TINY16_ASM_ABORTF("duplicated label: %s", tmp);
                }

                if (ctx.label_count >= MAX_LABELS) {
                    TINY16_ASM_ABORT("too many labels");
                }

                ctx.labels[ctx.label_count].addr = pc;

                strncpy(ctx.labels[ctx.label_count].name, line_buffer, lbl_len - 1);
                ctx.labels[ctx.label_count].name[lbl_len - 1] = '\0';
                ctx.label_count++;
                line_buffer += lbl_len;
                str_trim_left(line_buffer);
                if (strlen(line_buffer) == 0)
                    continue;
            }
            pc += 3;
        }

        //
        // pass 2
        //
        pc = 0;
        fseek(file, 0L, SEEK_SET);
        line_no = 0;
        while (fgets(buffer, LINE_BUFFER_SIZE, file) != NULL) {
            line_no++;

            char* line_buffer = buffer;

            strip_comment(line_buffer);
            str_trim_left(line_buffer);

            if (strlen(line_buffer) == 0)
                continue;

            int lbl_len = label_length(line_buffer);
            if (lbl_len > 0) {
                line_buffer += lbl_len;
                str_trim_left(line_buffer);
                if (strlen(line_buffer) == 0)
                    continue;
            }
            str_trim_right(line_buffer);
            output_instruction(&ctx, filename, line_no, line_buffer);
            pc += 3;
        }

        fclose(file);
        if (errno) {
            perror("could not close input file");
            return EXIT_FAILURE;
        }
    }

    fclose(ctx.out);
    if (errno) {
        perror("could not close output file");
        return EXIT_FAILURE;
    }

    return 0;
}

char* args_shift(int* argc, char*** argv) {
    assert(*argc > 0 && "argc <= 0");
    --(*argc);
    return *(*argv)++;
}

void strip_comment(char* str) {
    char* cut = strchr(str, ';');
    if (cut) {
        *cut = '\0';
    }
}

void str_trim_left(char* str) {
    char* start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

void str_trim_right(char* str) {
    if (*str == '\0')
        return;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;
    end[1] = '\0';
}

void str_trim(char* str) {
    str_trim_left(str);
    str_trim_right(str);
}

void str_to_upper(char* str) {
    while (*str != '\0') {
        *str = toupper(*str);
        str++;
    }
}

bool is_valid_label_prefix(char* str) {
    return (str && (isalpha(str[0]) || str[0] == '_' || str[0] == '.'));
}

int label_length(char* str) {
    int len = strlen(str);
    int i;
    if (!(is_valid_label_prefix(str))) {
        return 0;
    }
    for (i = 1; i < len; ++i) {
        bool valid = isalpha(str[i]) || isdigit(str[i]) || str[i] == '.' || str[i] == '_';
        if (!valid)
            break;
    }
    if (i > 0 && i < len && str[i] == ':') {
        return i + 1;
    }
    return 0;
}

long str_to_long(const char* str) {
    int base = 10;
    if (str[0] == '0') {
        switch (str[1]) {
        case 'b':
            str += 2;
            base = 2;
            break;
        case 'x':
            str += 2;
            base = 16;
            break;
        case '\0': break;
        default: errno = EINVAL; return 0;
        }
    }
    errno = 0;
    long val = strtol(str, NULL, base);
    if (errno)
        return 0;
    return val;
}

static char* token_separator = ", \t";

Tiny16OpCode expect_mnemonic(char* str, char** saveptr, char* filename, int line_no) {
    char* mnemonic = strtok_r(str, token_separator, saveptr);
    if (!mnemonic) {
        TINY16_ASM_ABORTF("could not parse mnemonic: %s", mnemonic);
    }
    str_to_upper(mnemonic);
    Tiny16OpCode opcode = tiny16_opcode_from_mnemonic(mnemonic);
    if (opcode == TINY16_OPCODE_UNKNOWN) {
        TINY16_ASM_ABORTF("unknown mnemonic: %s", mnemonic);
    }
    return opcode;
}

uint8_t expect_reg(char* str, char** saveptr, char* filename, int line_no) {
    char* reg = strtok_r(str, token_separator, saveptr);
    if (!reg) {
        TINY16_ASM_ABORTF("could not parse register: %s", reg);
    }
    if (strlen(reg) != 2) {
        TINY16_ASM_ABORTF("invalid register: %s", reg);
    }
    if (reg[0] != 'R' || (reg[1] < '0' || reg[1] > '7')) {
        TINY16_ASM_ABORTF("register not found, %s", reg);
    }
    return (uint8_t)(reg[1] - '0');
}

uint16_t expect_imm(AsmContext* ctx, char* str, char** saveptr, char* filename, int line_no) {
    char* imm = strtok_r(str, token_separator, saveptr);
    if (!imm) {
        TINY16_ASM_ABORTF("immediate expected, found: %s", imm);
    }
    if (isalpha(*imm)) {
        uint16_t addr = label_addr(ctx, imm);
        if (addr == LABEL_NOT_FOUND) {
            TINY16_ASM_ABORTF("label %s not found", imm);
        }
        return addr;
    }
    long val = str_to_long(imm);
    if (errno) {
        TINY16_ASM_ABORTF("invalid immediate: %s", imm);
    }
    if (val < 0 || val > UINT16_MAX) {
        TINY16_ASM_ABORTF("immediate out of bounds: %ld", val);
    }
    return (uint16_t)val;
}

uint8_t expect_imm8(AsmContext* ctx, char* str, char** saveptr, char* filename, int line_no) {
    uint16_t imm = expect_imm(ctx, str, saveptr, filename, line_no);
    if (imm > UINT8_MAX) {
        TINY16_ASM_ABORTF("immediate out of bounds: %" PRIu16, imm);
    }
    return (uint8_t)imm;
}

void output_instruction(AsmContext* ctx, char* filename, int line_no, char* line) {
    if ((ctx->out_size + 3) > TINY16_MEMORY_CODE_END) {
        TINY16_ASM_ABORT("max program size is 64KB");
    }

    char* saveptr;
    Tiny16OpCode opcode = expect_mnemonic(line, &saveptr, filename, line_no);

    uint8_t bytes[3];
    bytes[0] = opcode;

    switch (opcode) {
    case TINY16_OPCODE_LOADI:
        bytes[1] = expect_reg(NULL, &saveptr, filename, line_no);
        bytes[2] = expect_imm8(ctx, NULL, &saveptr, filename, line_no);
        break;

    case TINY16_OPCODE_ADD:
    case TINY16_OPCODE_SUB:
    case TINY16_OPCODE_AND:
    case TINY16_OPCODE_OR:
    case TINY16_OPCODE_XOR:
        bytes[1] = expect_reg(NULL, &saveptr, filename, line_no);
        bytes[2] = expect_reg(NULL, &saveptr, filename, line_no);
        break;

    case TINY16_OPCODE_LOAD:
    case TINY16_OPCODE_STORE:
    case TINY16_OPCODE_INC:
    case TINY16_OPCODE_DEC:
        bytes[1] = expect_reg(NULL, &saveptr, filename, line_no);
        bytes[2] = 0;
        break;

    case TINY16_OPCODE_JMP:
    case TINY16_OPCODE_JZ:
    case TINY16_OPCODE_JNZ: {
        uint16_t addr = expect_imm(ctx, NULL, &saveptr, filename, line_no);
        bytes[1] = (addr >> 8) & 0xFF;
        bytes[2] = addr & 0xFF;
    }; break;

    case TINY16_OPCODE_HALT:
        bytes[1] = 0;
        bytes[2] = 0;
        break;

    case TINY16_OPCODE_UNKNOWN: return;
    }

    if (strtok_r(NULL, token_separator, &saveptr) != NULL) {
        fprintf(stderr, "%s:%d: too many operands\n", filename, line_no);
        exit(1);
    }

    size_t n = fwrite(bytes, 1, 3, ctx->out);
    if (n != 3) {
        perror("write");
        exit(1);
    }

    ctx->out_size += 3;
}

uint16_t label_addr(AsmContext* ctx, char* name) {
    for (int i = 0; i < ctx->label_count; ++i) {
        if (strcmp(ctx->labels[i].name, name) == 0)
            return ctx->labels[i].addr;
    }
    return LABEL_NOT_FOUND;
}
