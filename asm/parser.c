#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
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

static const char* tiny16_parser_error_messages[] = {
    [TINY16_PARSER_OK] = "",
    [TINY16_PARSER_ERROR_UNEXPECTED_TOKEN] = "expected %s, found %s",
    [TINY16_PARSER_ERROR_LABEL_TOO_LONG] = "label name too long: %zu (max %d)",
    [TINY16_PARSER_ERROR_DUPLICATE_LABEL] = "duplicate label: %.*s",
    [TINY16_PARSER_ERROR_TOO_MANY_LABELS] = "too many labels",
    [TINY16_PARSER_ERROR_UNKNOWN_SECTION] = "unknown section: %.*s",
    [TINY16_PARSER_ERROR_INVALID_NUMBER] = "invalid number: %s",
    [TINY16_PARSER_ERROR_OUT_OF_RANGE] = "%s out of range: %ld",
    [TINY16_PARSER_ERROR_UNKNOWN_MNEMONIC] = "unknown mnemonic: %s",
    [TINY16_PARSER_ERROR_INVALID_REGISTER] = "invalid register: %.*s",
    [TINY16_PARSER_ERROR_EXPECTED_EVEN_REGISTER] = "expected even register, found R%d",
    [TINY16_PARSER_ERROR_WRONG_REGISTER] = "expected R%d, found R%d",
    [TINY16_PARSER_ERROR_UNDEFINED_LABEL] = "undefined label: %.*s",
    [TINY16_PARSER_ERROR_UNKNOWN_DIRECTIVE] = "unknown directive: %.*s",
    [TINY16_PARSER_ERROR_PROGRAM_TOO_LARGE] = "max program size is %d bytes",
    [TINY16_PARSER_ERROR_ORG_REWIND_NOT_ALLOWED] =
        "org rewind to 0x%04X not allowed, data at 0x%04X",
};

static void tiny16_parser_set_error(Tiny16Parser* parser, Tiny16ParserError error, ...) {
    if (parser->error != TINY16_PARSER_OK) return; // keep first error

    parser->error = error;
    parser->error_line = parser->current_token.line;

    va_list args;
    va_start(args, error);
    vsnprintf(parser->error_msg, TINY16_PARSER_MAX_ERROR_MSG, tiny16_parser_error_messages[error],
              args);
    va_end(args);
}

bool tiny16_parser_has_error(const Tiny16Parser* parser) {
    return parser->error != TINY16_PARSER_OK;
}

void tiny16_parser_print_error(const Tiny16Parser* parser) {
    if (parser->error != TINY16_PARSER_OK) {
        fprintf(stderr, "%s:%zu: %s\n", parser->source_filename, parser->error_line,
                parser->error_msg);
    }
}

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
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_UNEXPECTED_TOKEN, token_kind_name(kind),
                                token_kind_name(parser->current_token.kind));
        return;
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
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_LABEL_TOO_LONG, label_token.text_len,
                                TINY16_PARSER_MAX_TOKEN_LENGTH);
        return true;
    }

    if (tiny16_parser_label_addr(parser, label_token.text, label_token.text_len) !=
        TINY16_PARSER_LABEL_NOT_FOUND) {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_DUPLICATE_LABEL,
                                (int)label_token.text_len, label_token.text);
        return true;
    }

    if (parser->label_count >= TINY16_PARSER_MAX_LABELS) {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_TOO_MANY_LABELS);
        return true;
    }

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
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_UNEXPECTED_TOKEN,
                                token_kind_name(TOKEN_SYMBOL),
                                token_kind_name(parser->current_token.kind));
        return true;
    }

    Token section_name = parser->current_token;
    tiny16_parser_next(parser);
    if (section_name.text_len == 4 && str_eq_ci(section_name.text, "code", 4)) {
        parser->current_section = TINY16_PARSER_SECTION_CODE;
    } else if (section_name.text_len == 4 && str_eq_ci(section_name.text, "data", 4)) {
        parser->current_section = TINY16_PARSER_SECTION_DATA;
    } else {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_UNKNOWN_SECTION,
                                (int)section_name.text_len, section_name.text);
    }

    return true;
}

bool tiny16_parser_parse_org(Tiny16Parser* parser) {
    if (parser->current_token.kind != TOKEN_KEYWORD) return false;
    if (!str_eq_ci(parser->current_token.text, "org", parser->current_token.text_len)) return false;

    tiny16_parser_next(parser);
    if (parser->current_token.kind != TOKEN_NUMBER) {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_UNEXPECTED_TOKEN,
                                token_kind_name(TOKEN_NUMBER),
                                token_kind_name(parser->current_token.kind));
        return true;
    }

    long addr16 = strtol(parser->current_token.text, NULL, 0);
    if (addr16 < TINY16_DATA_BEGIN || addr16 > TINY16_DATA_END) {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_OUT_OF_RANGE, "ORG addr16", addr16);
        return true;
    }

    uint16_t target = addr16 - TINY16_DATA_BEGIN;
    if (target < parser->data_size) {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_ORG_REWIND_NOT_ALLOWED,
                                (unsigned)addr16,
                                (unsigned)(TINY16_DATA_BEGIN + parser->data_size));
        return true;
    }

    while (parser->data_size < target) {
        parser->data_buffer[parser->data_size++] = 0;
    }

    parser->data_pc = TINY16_DATA_BEGIN + parser->data_size;
    tiny16_parser_next(parser); // consume addr16

    return true;
}

uint16_t tiny16_parser_parse_times_prefix(Tiny16Parser* parser) {
    if (parser->current_token.kind != TOKEN_KEYWORD) return 1;

    if (!str_eq_ci(parser->current_token.text, "times", parser->current_token.text_len)) return 1;
    tiny16_parser_next(parser); // 'times'

    if (parser->current_token.kind != TOKEN_NUMBER) {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_UNEXPECTED_TOKEN,
                                token_kind_name(TOKEN_NUMBER),
                                token_kind_name(parser->current_token.kind));
        return 1;
    }

    long count = strtol(parser->current_token.text, NULL, 0);
    if (count < 0 || count > UINT16_MAX) {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_OUT_OF_RANGE, "TIMES count", count);
        return 1;
    }

    tiny16_parser_next(parser);
    return (uint16_t)count;
}

Tiny16OpCode tiny16_parser_parse_mnemonic(Tiny16Parser* parser) {
    if (parser->current_token.kind != TOKEN_SYMBOL) {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_UNEXPECTED_TOKEN,
                                token_kind_name(TOKEN_SYMBOL),
                                token_kind_name(parser->current_token.kind));
        return TINY16_OPCODE_UNKNOWN;
    }

    char mnemonic[TINY16_PARSER_MAX_TOKEN_LENGTH];
    size_t len = parser->current_token.text_len;
    if (len >= TINY16_PARSER_MAX_TOKEN_LENGTH) len = TINY16_PARSER_MAX_TOKEN_LENGTH - 1;
    strncpy(mnemonic, parser->current_token.text, len);
    mnemonic[len] = '\0';

    Tiny16OpCode opcode = tiny16_opcode_from_mnemonic(mnemonic);
    if (opcode == TINY16_OPCODE_UNKNOWN) {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_UNKNOWN_MNEMONIC, mnemonic);
        return TINY16_OPCODE_UNKNOWN;
    }

    tiny16_parser_next(parser);
    return opcode;
}

uint8_t tiny16_parser_parse_reg(Tiny16Parser* parser) {
    if (parser->current_token.kind != TOKEN_SYMBOL) {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_UNEXPECTED_TOKEN,
                                token_kind_name(TOKEN_SYMBOL),
                                token_kind_name(parser->current_token.kind));
        return 0;
    }

    if (parser->current_token.text_len != 2 ||
        tolower((unsigned char)parser->current_token.text[0]) != 'r' ||
        parser->current_token.text[1] < '0' || parser->current_token.text[1] > '7') {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_INVALID_REGISTER,
                                (int)parser->current_token.text_len, parser->current_token.text);
        return 0;
    }

    uint8_t reg = parser->current_token.text[1] - '0';
    tiny16_parser_next(parser);
    return reg;
}

Tiny16AddrPair tiny16_parser_parse_reg_pair(Tiny16Parser* parser) {
    uint8_t rh = tiny16_parser_parse_reg(parser);
    if (rh % 2 != 0) {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_EXPECTED_EVEN_REGISTER, rh);
        return 0;
    }

    tiny16_parser_expect(parser, TOKEN_COLON);
    uint8_t rl = tiny16_parser_parse_reg(parser);
    if (rl != rh + 1) {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_WRONG_REGISTER, rh + 1, rl);
        return 0;
    }

    return (Tiny16AddrPair)(rh / 2);
}

uint16_t tiny16_parser_parse_imm(Tiny16Parser* parser) {
    if (parser->current_token.kind == TOKEN_SYMBOL) {
        uint16_t addr = tiny16_parser_label_addr(parser, parser->current_token.text,
                                                 parser->current_token.text_len);
        if (addr == TINY16_PARSER_LABEL_NOT_FOUND) {
            tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_UNDEFINED_LABEL,
                                    (int)parser->current_token.text_len,
                                    parser->current_token.text);
            return 0;
        }
        tiny16_parser_next(parser);
        return addr;
    }

    if (parser->current_token.kind == TOKEN_NUMBER) {
        long val = strtol(parser->current_token.text, NULL, 0);
        if (val < 0 || val > UINT16_MAX) {
            tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_OUT_OF_RANGE, "immediate", val);
            return 0;
        }
        tiny16_parser_next(parser);
        return (uint16_t)val;
    }

    tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_UNEXPECTED_TOKEN,
                            token_kind_name(TOKEN_NUMBER),
                            token_kind_name(parser->current_token.kind));
    return 0;
}

uint8_t tiny16_parser_parse_imm8(Tiny16Parser* parser) {
    uint16_t imm = tiny16_parser_parse_imm(parser);
    if (imm > UINT8_MAX) {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_OUT_OF_RANGE, "8-bit immediate",
                                (long)imm);
        return 0;
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
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_UNEXPECTED_TOKEN,
                                token_kind_name(TOKEN_KEYWORD),
                                token_kind_name(parser->current_token.kind));
        return;
    }

    if (!str_eq_ci(parser->current_token.text, "db", parser->current_token.text_len)) {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_UNKNOWN_DIRECTIVE,
                                (int)parser->current_token.text_len, parser->current_token.text);
        return;
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
                tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_OUT_OF_RANGE, "byte value",
                                        val);
                return;
            }
            parser->data_buffer[parser->data_size++] = (uint8_t)val;
            tiny16_parser_next(parser);

        } else {
            tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_UNEXPECTED_TOKEN,
                                    token_kind_name(TOKEN_STRING),
                                    token_kind_name(parser->current_token.kind));
            return;
        }

        tiny16_parser_match(parser, TOKEN_COMMA); // optional comma
    }

    parser->data_pc = TINY16_MEMORY_DATA_BEGIN + parser->data_size;
}

void tiny16_parser_emit_code(Tiny16Parser* parser) {
    const uint16_t max_program_size = TINY16_MEMORY_CODE_END - TINY16_MEMORY_CODE_BEGIN;
    if ((parser->output_file_size + 3) > max_program_size) {
        tiny16_parser_set_error(parser, TINY16_PARSER_ERROR_PROGRAM_TOO_LARGE, max_program_size);
        return;
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

    if (tiny16_parser_has_error(parser)) return;

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
