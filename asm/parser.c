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
#include "lexer.h"
#include "memory.h"
#include "parser.h"

#define TINY16_PARSER_ABORT(parser, fmt)                                                           \
    do {                                                                                           \
        fprintf(stderr, "%s:%zu: " fmt "\n", (parser)->source_filename,                            \
                (parser)->current_token.line);                                                     \
        exit(1);                                                                                   \
    } while (0)

#define TINY16_PARSER_ABORTF(parser, fmt, ...)                                                     \
    do {                                                                                           \
        fprintf(stderr, "%s:%zu: " fmt "\n", (parser)->source_filename,                            \
                (parser)->current_token.line, __VA_ARGS__);                                        \
        exit(1);                                                                                   \
    } while (0)

void tiny16_parser_next(Tiny16Parser* parser) {
    parser->current_token = lexer_next(&parser->lexer);
}

void tiny16_parser_skip_trivia(Tiny16Parser* parser) {
    while (parser->current_token.kind == TOKEN_COMMENT || parser->current_token.kind == TOKEN_EOL) {
        tiny16_parser_next(parser);
    }
}

void tiny16_parser_expect(Tiny16Parser* parser, TokenKind kind) {
    if (parser->current_token.kind != kind) {
        TINY16_PARSER_ABORTF(parser, "expected %s, found %s", token_kind_name(kind),
                             token_kind_name(parser->current_token.kind));
    }
    tiny16_parser_next(parser);
}

bool tiny16_parser_match(Tiny16Parser* parser, TokenKind kind) {
    if (parser->current_token.kind == kind) {
        tiny16_parser_next(parser);
        return true;
    }
    return false;
}

void tiny16_parser_skip_to_eol(Tiny16Parser* parser) {
    while (parser->current_token.kind != TOKEN_EOL && parser->current_token.kind != TOKEN_END) {
        tiny16_parser_next(parser);
    }
    if (parser->current_token.kind == TOKEN_EOL) tiny16_parser_next(parser);
}

bool tiny16_parser_parse_label(Tiny16Parser* parser) {
    if (parser->current_token.kind != TOKEN_SYMBOL) return false;

    Lexer saved_lexer = parser->lexer;
    Token next_token = lexer_next(&parser->lexer);
    if (next_token.kind != TOKEN_COLON) {
        parser->lexer = saved_lexer;
        return false;
    }

    Token label_token = parser->current_token;
    parser->current_token = next_token; // move to ':'
    tiny16_parser_next(parser);         // advance past ':'

    if (label_token.text_len >= TINY16_PARSER_MAX_TOKEN_LENGTH) {
        TINY16_PARSER_ABORTF(parser, "label name too long: %zu (max %d)", label_token.text_len,
                             TINY16_PARSER_MAX_TOKEN_LENGTH);
    }

    if (tiny16_parser_label_addr(parser, label_token.text, label_token.text_len) !=
        TINY16_PARSER_LABEL_NOT_FOUND) {
        TINY16_PARSER_ABORTF(parser, "duplicate label: %.*s", (int)label_token.text_len,
                             label_token.text);
    }

    if (parser->label_count >= TINY16_PARSER_MAX_LABELS)
        TINY16_PARSER_ABORT(parser, "too many labels");

    uint16_t addr =
        (parser->current_section == TINY16_PARSER_SECTION_CODE) ? parser->code_pc : parser->data_pc;

    strncpy(parser->labels[parser->label_count].name, label_token.text, label_token.text_len);
    parser->labels[parser->label_count].name[label_token.text_len] = '\0';
    parser->labels[parser->label_count].addr = addr;
    parser->label_count++;

    return true;
}

bool tiny16_parser_skip_label(Tiny16Parser* parser) {
    if (parser->current_token.kind != TOKEN_SYMBOL) return false;

    Lexer saved_lexer = parser->lexer;
    Token next_token = lexer_next(&parser->lexer);
    if (next_token.kind != TOKEN_COLON) {
        parser->lexer = saved_lexer;
        return false;
    }

    parser->current_token = next_token; // move to ':'
    tiny16_parser_next(parser);         // advance past ':'

    return true;
}

uint16_t tiny16_parser_label_addr(Tiny16Parser* parser, const char* name, size_t len) {
    for (int i = 0; i < parser->label_count; ++i) {
        size_t label_len = strlen(parser->labels[i].name);
        if (label_len == len && memcmp(parser->labels[i].name, name, len) == 0) {
            return parser->labels[i].addr;
        }
    }
    return TINY16_PARSER_LABEL_NOT_FOUND;
}

bool tiny16_parser_parse_section(Tiny16Parser* parser) {
    if (parser->current_token.kind != TOKEN_KEYWORD) return false;
    if (!str_eq_ci(parser->current_token.text, "section", parser->current_token.text_len))
        return false;
    tiny16_parser_next(parser);

    tiny16_parser_expect(parser, TOKEN_DOT);
    if (parser->current_token.kind != TOKEN_SYMBOL) {
        TINY16_PARSER_ABORT(parser, "expected section name after 'section'");
    }

    Token section_name = parser->current_token;
    tiny16_parser_next(parser);
    if (section_name.text_len == 4 && str_eq_ci(section_name.text, "code", 4)) {
        parser->current_section = TINY16_PARSER_SECTION_CODE;
    } else if (section_name.text_len == 4 && str_eq_ci(section_name.text, "data", 4)) {
        parser->current_section = TINY16_PARSER_SECTION_DATA;
    } else {
        TINY16_PARSER_ABORTF(parser, "unknown section: %.*s", (int)section_name.text_len,
                             section_name.text);
    }

    return true;
}

uint16_t tiny16_parser_parse_times_prefix(Tiny16Parser* parser) {
    if (parser->current_token.kind != TOKEN_KEYWORD) return 1;

    if (!str_eq_ci(parser->current_token.text, "times", parser->current_token.text_len)) return 1;
    tiny16_parser_next(parser); // 'times'

    if (parser->current_token.kind != TOKEN_NUMBER) {
        TINY16_PARSER_ABORT(parser, "expected number after TIMES");
    }

    long count = strtol(parser->current_token.text, NULL, 0);
    if (count < 0 || count > UINT16_MAX) {
        TINY16_PARSER_ABORT(parser, "TIMES count out of range");
    }

    tiny16_parser_next(parser);
    return (uint16_t)count;
}

Tiny16OpCode tiny16_parser_parse_mnemonic(Tiny16Parser* parser) {
    if (parser->current_token.kind != TOKEN_SYMBOL) {
        TINY16_PARSER_ABORT(parser, "expected mnemonic");
    }

    char mnemonic[TINY16_PARSER_MAX_TOKEN_LENGTH];
    size_t len = parser->current_token.text_len;
    if (len >= TINY16_PARSER_MAX_TOKEN_LENGTH) len = TINY16_PARSER_MAX_TOKEN_LENGTH - 1;
    strncpy(mnemonic, parser->current_token.text, len);
    mnemonic[len] = '\0';

    Tiny16OpCode opcode = tiny16_opcode_from_mnemonic(mnemonic);
    if (opcode == TINY16_OPCODE_UNKNOWN) {
        TINY16_PARSER_ABORTF(parser, "unknown mnemonic: %s", mnemonic);
    }

    tiny16_parser_next(parser);
    return opcode;
}

uint8_t tiny16_parser_parse_reg(Tiny16Parser* parser) {
    if (parser->current_token.kind != TOKEN_SYMBOL) {
        TINY16_PARSER_ABORT(parser, "expected register");
    }

    if (parser->current_token.text_len != 2 ||
        tolower((unsigned char)parser->current_token.text[0]) != 'r' ||
        parser->current_token.text[1] < '0' || parser->current_token.text[1] > '7') {
        TINY16_PARSER_ABORTF(parser, "invalid register: %.*s", (int)parser->current_token.text_len,
                             parser->current_token.text);
    }

    uint8_t reg = parser->current_token.text[1] - '0';
    tiny16_parser_next(parser);
    return reg;
}

Tiny16AddrPair tiny16_parser_parse_reg_pair(Tiny16Parser* parser) {
    uint8_t rh = tiny16_parser_parse_reg(parser);
    if (rh % 2 != 0) {
        TINY16_PARSER_ABORTF(parser, "expected even register, found R%d", rh);
    }

    tiny16_parser_expect(parser, TOKEN_COLON);

    uint8_t rl = tiny16_parser_parse_reg(parser);
    if (rl != rh + 1) {
        TINY16_PARSER_ABORTF(parser, "expected R%d, found R%d", rh + 1, rl);
    }

    return (Tiny16AddrPair)(rh / 2);
}

uint16_t tiny16_parser_parse_imm(Tiny16Parser* parser) {
    if (parser->current_token.kind == TOKEN_SYMBOL) {
        uint16_t addr = tiny16_parser_label_addr(parser, parser->current_token.text,
                                                 parser->current_token.text_len);
        if (addr == TINY16_PARSER_LABEL_NOT_FOUND) {
            TINY16_PARSER_ABORTF(parser, "undefined label: %.*s",
                                 (int)parser->current_token.text_len, parser->current_token.text);
        }
        tiny16_parser_next(parser);
        return addr;
    }

    if (parser->current_token.kind == TOKEN_NUMBER) {
        long val = strtol(parser->current_token.text, NULL, 0);
        if (val < 0 || val > UINT16_MAX) {
            TINY16_PARSER_ABORTF(parser, "immediate out of range: %ld", val);
        }
        tiny16_parser_next(parser);
        return (uint16_t)val;
    }

    TINY16_PARSER_ABORT(parser, "expected number or label");
    return 0;
}

uint8_t tiny16_parser_parse_imm8(Tiny16Parser* parser) {
    uint16_t imm = tiny16_parser_parse_imm(parser);
    if (imm > UINT8_MAX) {
        TINY16_PARSER_ABORTF(parser, "immediate out of 8-bit range: %u", imm);
    }
    return (uint8_t)imm;
}

Tiny16Addr tiny16_parser_parse_addr(Tiny16Parser* parser) {
    Tiny16Addr addr = {0};

    addr.reg = tiny16_parser_parse_reg(parser);
    tiny16_parser_match(parser, TOKEN_COMMA); // optional

    tiny16_parser_expect(parser, TOKEN_OPEN_BRACKET);

    addr.pair = tiny16_parser_parse_reg_pair(parser);
    addr.mode = TINY16_ADDR_MODE_BASE;
    addr.offset = 0;

    if (tiny16_parser_match(parser, TOKEN_PLUS)) {
        addr.mode = TINY16_ADDR_MODE_OFFSET;
        addr.offset = tiny16_parser_parse_imm8(parser);
    }

    tiny16_parser_expect(parser, TOKEN_CLOSE_BRACKET);

    if (addr.mode == TINY16_ADDR_MODE_BASE) {
        if (tiny16_parser_match(parser, TOKEN_PLUS)) {
            addr.mode = TINY16_ADDR_MODE_INC;
        } else if (tiny16_parser_match(parser, TOKEN_MINUS)) {
            addr.mode = TINY16_ADDR_MODE_DEC;
        }
    }

    return addr;
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

void tiny16_parser_parse_data(Tiny16Parser* parser) {
    if (parser->current_token.kind != TOKEN_KEYWORD) {
        TINY16_PARSER_ABORT(parser, "expected data directive");
    }

    if (!str_eq_ci(parser->current_token.text, "db", parser->current_token.text_len)) {
        TINY16_PARSER_ABORTF(parser, "unknown directive: %.*s", (int)parser->current_token.text_len,
                             parser->current_token.text);
    }

    tiny16_parser_next(parser); // 'db'

    while (parser->current_token.kind != TOKEN_EOL && parser->current_token.kind != TOKEN_END &&
           parser->current_token.kind != TOKEN_COMMENT) {
        if (parser->current_token.kind == TOKEN_STRING) {
            const char* str = parser->current_token.text + 1; // skip opening "
            size_t len = parser->current_token.text_len - 2;  // skip both quotes
            for (size_t i = 0; i < len; ++i) {
                if (str[i] == '\\' && i + 1 < len) {
                    parser->data_buffer[parser->data_size++] = tiny16_parser_parse_escape(str[++i]);
                } else {
                    parser->data_buffer[parser->data_size++] = str[i];
                }
            }
            tiny16_parser_next(parser);

        } else if (parser->current_token.kind == TOKEN_NUMBER) {
            long val = strtol(parser->current_token.text, NULL, 0);
            if (val < 0 || val > 255) {
                TINY16_PARSER_ABORTF(parser, "byte value out of range: %ld", val);
            }
            parser->data_buffer[parser->data_size++] = (uint8_t)val;
            tiny16_parser_next(parser);

        } else {
            TINY16_PARSER_ABORTF(parser, "expected string or number, found %s",
                                 token_kind_name(parser->current_token.kind));
        }

        tiny16_parser_match(parser, TOKEN_COMMA); // optional comma
    }

    parser->data_pc = TINY16_MEMORY_DATA_BEGIN + parser->data_size;
}

void tiny16_parser_emit_code(Tiny16Parser* parser) {
    const uint16_t max_program_size = TINY16_MEMORY_CODE_END - TINY16_MEMORY_CODE_BEGIN;
    if ((parser->output_file_size + 3) > max_program_size) {
        TINY16_PARSER_ABORTF(parser, "max program size is %d bytes", max_program_size);
    }

    Tiny16OpCode opcode = tiny16_parser_parse_mnemonic(parser);

    uint8_t bytes[3];
    bytes[0] = opcode;

    switch (opcode) {
    case TINY16_OPCODE_LOADI:
        bytes[1] = tiny16_parser_parse_reg(parser);
        tiny16_parser_expect(parser, TOKEN_COMMA);
        bytes[2] = tiny16_parser_parse_imm8(parser);
        break;

    case TINY16_OPCODE_LOAD:
    case TINY16_OPCODE_STORE: {
        Tiny16Addr addr = tiny16_parser_parse_addr(parser);
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
        bytes[1] = tiny16_parser_parse_reg(parser);
        tiny16_parser_expect(parser, TOKEN_COMMA);
        bytes[2] = tiny16_parser_parse_reg(parser);
        break;

    case TINY16_OPCODE_INC:
    case TINY16_OPCODE_DEC:
    case TINY16_OPCODE_SHL:
    case TINY16_OPCODE_SHR:
    case TINY16_OPCODE_PUSH:
    case TINY16_OPCODE_POP:
        bytes[1] = tiny16_parser_parse_reg(parser);
        bytes[2] = 0;
        break;

    case TINY16_OPCODE_MOVSPR:
    case TINY16_OPCODE_MOVRSP:
        bytes[1] = tiny16_parser_parse_reg_pair(parser);
        bytes[2] = 0;
        break;

    case TINY16_OPCODE_JMP:
    case TINY16_OPCODE_JZ:
    case TINY16_OPCODE_JNZ:
    case TINY16_OPCODE_JC:
    case TINY16_OPCODE_JNC:
    case TINY16_OPCODE_CALL: {
        uint16_t addr = tiny16_parser_parse_imm(parser);
        bytes[1] = (addr >> 8) & 0xFF;
        bytes[2] = addr & 0xFF;
    } break;

    case TINY16_OPCODE_RET:
    case TINY16_OPCODE_HALT:
        bytes[1] = 0;
        bytes[2] = 0;
        break;

    case TINY16_OPCODE_UNKNOWN:
        return;
    }

    size_t n = fwrite(bytes, 1, 3, parser->output_file);
    if (n != 3) {
        perror("write code");
        exit(1);
    }

    parser->output_file_size += 3;
}

void tiny16_parser_emit_data(Tiny16Parser* parser) {
    size_t n = TINY16_MEMORY_CODE_END - parser->code_pc + 1;
    for (size_t i = 0; i < n; ++i)
        fputc('\0', parser->output_file);
    parser->output_file_size += n;

    n = fwrite(parser->data_buffer, 1, parser->data_size, parser->output_file);
    if (n != parser->data_size) {
        perror("write data");
        exit(1);
    }
}
