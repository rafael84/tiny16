#include "preprocessor.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const char* tiny16_pp_error_messages[] = {
    [TINY16_PP_OK] = "",
    [TINY16_PP_ERROR_FILE_NOT_FOUND] = "file not found: %s",
    [TINY16_PP_ERROR_INCLUDE_DEPTH_EXCEEDED] = "include depth exceeded (max %d)",
    [TINY16_PP_ERROR_CIRCULAR_INCLUDE] = "circular include detected: %s",
    [TINY16_PP_ERROR_MACRO_TOO_LARGE] = "macro too large (max %d lines)",
    [TINY16_PP_ERROR_TOO_MANY_MACROS] = "too many macros (max %d)",
    [TINY16_PP_ERROR_TOO_MANY_PARAMS] = "too many parameters (max %d)",
    [TINY16_PP_ERROR_DUPLICATE_MACRO] = "duplicate macro: %s",
    [TINY16_PP_ERROR_MISMATCHED_ENDMACRO] = "mismatched .endmacro",
    [TINY16_PP_ERROR_UNEXPECTED_ENDMACRO] = "unexpected .endmacro without .macro",
    [TINY16_PP_ERROR_ARGUMENT_COUNT_MISMATCH] = "expected %d arguments, found %d",
    [TINY16_PP_ERROR_INVALID_INCLUDE] = "invalid .include directive",
};

static char* tiny16_strdup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* out = malloc(n);
    if (!out) return NULL;
    memcpy(out, s, n);
    return out;
}

static void tiny16_pp_set_error(Tiny16Preprocessor* pp, Tiny16PPError error, ...) {
    if (pp->error != TINY16_PP_OK) return; // keep first error

    pp->error = error;

    va_list args;
    va_start(args, error);
    vsnprintf(pp->error_msg, TINY16_PP_MAX_ERROR_MSG, tiny16_pp_error_messages[error], args);
    va_end(args);
}

void tiny16_pp_init(Tiny16Preprocessor* pp) { memset(pp, 0, sizeof(Tiny16Preprocessor)); }

void tiny16_pp_free(Tiny16Preprocessor* pp) {
    for (int i = 0; i < pp->macro_count; i++) {
        for (int j = 0; j < pp->macros[i].line_count; j++) {
            free(pp->macros[i].lines[j]);
        }
    }
}

bool tiny16_pp_has_error(const Tiny16Preprocessor* pp) { return pp->error != TINY16_PP_OK; }

void tiny16_pp_print_error(const Tiny16Preprocessor* pp) {
    if (pp->error != TINY16_PP_OK) {
        fprintf(stderr, "%s:%zu: %s\n", pp->error_file, pp->error_line, pp->error_msg);
    }
}

static bool tiny16_pp_at_end(Tiny16Preprocessor* pp) { return pp->cursor >= pp->content_len; }

static void tiny16_pp_read_line(Tiny16Preprocessor* pp) {
    int len = 0;
    while (pp->cursor < pp->content_len && len < TINY16_PP_MAX_LINE_LEN - 1) {
        char c = pp->content[pp->cursor++];
        pp->current_line[len++] = c;
        if (c == '\n') {
            pp->line++;
            break;
        }
    }
    pp->current_line[len] = '\0';
}

static void tiny16_pp_skip_whitespace(const char** p) {
    while (**p && isspace((unsigned char)**p) && **p != '\n')
        (*p)++;
}

static void tiny16_pp_skip_char(const char** p, char c) {
    if (**p == c) (*p)++;
}

static bool tiny16_pp_at_eol(const char* p) { return *p == '\0' || *p == ';' || *p == '\n'; }

static bool tiny16_pp_is_token_char(char c) {
    return !isspace((unsigned char)c) && c != ',' && c != ';' && c != '\n';
}

static int tiny16_pp_read_token(const char** p, char* buffer, int max_len) {
    int len = 0;
    while (**p && tiny16_pp_is_token_char(**p) && len < max_len - 1) {
        buffer[len++] = *(*p)++;
    }
    buffer[len] = '\0';
    return len;
}

static bool tiny16_pp_is_word_boundary(char c) {
    return isspace(c) || c == ',' || c == ':' || c == '[' || c == ']' || c == '+' || c == '-' ||
           c == '*' || c == '/' || c == '&' || c == '|' || c == '^' || c == '(' || c == ')' ||
           c == '\0';
}

static Tiny16Macro* tiny16_pp_find_macro(Tiny16Preprocessor* pp, const char* name) {
    for (int i = 0; i < pp->macro_count; i++) {
        if (strcasecmp(pp->macros[i].name, name) == 0) {
            return &pp->macros[i];
        }
    }
    return NULL;
}

static void tiny16_pp_parse_macro_header(Tiny16Preprocessor* pp) {
    const char* p = pp->current_line + 6;
    tiny16_pp_skip_whitespace(&p);

    char name[TINY16_PP_MAX_NAME_LEN];
    int name_len = tiny16_pp_read_token(&p, name, TINY16_PP_MAX_NAME_LEN);

    if (name_len == 0) return;
    if (tiny16_pp_find_macro(pp, name)) {
        tiny16_pp_set_error(pp, TINY16_PP_ERROR_DUPLICATE_MACRO, name);
        return;
    }

    if (pp->macro_count >= TINY16_PP_MAX_MACROS) {
        tiny16_pp_set_error(pp, TINY16_PP_ERROR_TOO_MANY_MACROS, TINY16_PP_MAX_MACROS);
        return;
    }

    Tiny16Macro* macro = &pp->macros[pp->macro_count++];
    strcpy(macro->name, name);
    macro->param_count = 0;
    macro->line_count = 0;
    strncpy(macro->file, pp->error_file, sizeof(macro->file) - 1);
    macro->file[sizeof(macro->file) - 1] = '\0';

    tiny16_pp_skip_whitespace(&p);

    while (!tiny16_pp_at_eol(p)) {
        tiny16_pp_skip_whitespace(&p);
        if (tiny16_pp_at_eol(p)) break;

        if (macro->param_count >= TINY16_PP_MAX_PARAMS) {
            tiny16_pp_set_error(pp, TINY16_PP_ERROR_TOO_MANY_PARAMS, TINY16_PP_MAX_PARAMS);
            return;
        }

        char* param = macro->params[macro->param_count];
        int param_len = tiny16_pp_read_token(&p, param, TINY16_PP_MAX_NAME_LEN);

        if (param_len > 0) macro->param_count++;

        tiny16_pp_skip_whitespace(&p);
        tiny16_pp_skip_char(&p, ',');
    }

    pp->current_macro = macro;
    pp->in_macro_def = true;
}

static bool tiny16_pp_is_macro_call(Tiny16Preprocessor* pp, Tiny16Macro** macro_out) {
    const char* p = pp->current_line;
    tiny16_pp_skip_whitespace(&p);

    if (tiny16_pp_at_eol(p)) return false;

    char token[TINY16_PP_MAX_NAME_LEN];
    tiny16_pp_read_token(&p, token, TINY16_PP_MAX_NAME_LEN);

    Tiny16Macro* macro = tiny16_pp_find_macro(pp, token);
    if (macro) {
        *macro_out = macro;
        return true;
    }
    return false;
}

static void tiny16_pp_parse_macro_args(Tiny16Preprocessor* pp) {
    const char* p = pp->current_line;
    tiny16_pp_skip_whitespace(&p);

    while (*p && tiny16_pp_is_token_char(*p))
        p++;

    tiny16_pp_skip_whitespace(&p);

    pp->temp_arg_count = 0;
    while (!tiny16_pp_at_eol(p)) {
        tiny16_pp_skip_whitespace(&p);
        if (tiny16_pp_at_eol(p)) break;

        tiny16_pp_skip_char(&p, ',');
        tiny16_pp_skip_whitespace(&p);

        char* arg = pp->temp_args[pp->temp_arg_count];
        int arg_len = 0;

        while (*p && *p != ',' && !tiny16_pp_at_eol(p) && arg_len < TINY16_PP_MAX_NAME_LEN - 1) {
            arg[arg_len++] = *p++;
        }

        while (arg_len > 0 && isspace((unsigned char)arg[arg_len - 1])) {
            arg_len--;
        }
        arg[arg_len] = '\0';

        if (arg_len > 0) {
            pp->temp_arg_count++;
        }
    }
}

static void tiny16_pp_replace_param(const char* line, const char* param, const char* arg,
                                    char* output, size_t output_size) {
    const char* p = line;
    size_t out_pos = 0;
    size_t param_len = strlen(param);

    while (*p && out_pos < output_size - 1) {
        if (strncmp(p, param, param_len) == 0) {
            char before = (p > line) ? *(p - 1) : ' ';
            char after = *(p + param_len);

            if (tiny16_pp_is_word_boundary(before) && tiny16_pp_is_word_boundary(after)) {
                size_t arg_len = strlen(arg);
                if (out_pos + arg_len < output_size - 1) {
                    strcpy(output + out_pos, arg);
                    out_pos += arg_len;
                    p += param_len;
                    continue;
                }
            }
        }
        output[out_pos++] = *p++;
    }
    output[out_pos] = '\0';
}

static void tiny16_pp_expand_macro(Tiny16Preprocessor* pp, Tiny16Macro* macro) {
    tiny16_pp_parse_macro_args(pp);

    if (pp->temp_arg_count != macro->param_count) {
        tiny16_pp_set_error(pp, TINY16_PP_ERROR_ARGUMENT_COUNT_MISMATCH, macro->param_count,
                            pp->temp_arg_count);
        return;
    }

    pp->macro_invocation_counter++;
    const char* call_file = pp->error_file;

    for (int i = 0; i < macro->line_count; i++) {
        char expanded[TINY16_PP_MAX_LINE_LEN];
        strcpy(expanded, macro->lines[i]);

        for (int p = 0; p < macro->param_count; p++) {
            tiny16_pp_replace_param(expanded, macro->params[p], pp->temp_args[p], pp->temp_buffer,
                                    sizeof(pp->temp_buffer));
            strcpy(expanded, pp->temp_buffer);
        }

        char* at = expanded;
        while ((at = strchr(at, '@')) != NULL) {
            if (at == expanded || isspace((unsigned char)*(at - 1))) {
                char label[TINY16_PP_MAX_NAME_LEN];
                int label_len = 0;
                const char* p = at + 1;
                while (*p && (isalnum((unsigned char)*p) || *p == '_') &&
                       label_len < TINY16_PP_MAX_NAME_LEN - 1) {
                    label[label_len++] = *p++;
                }
                label[label_len] = '\0';

                if (label_len > 0) {
                    char unique[TINY16_PP_MAX_LINE_LEN];
                    snprintf(unique, sizeof(unique), "__%s_%d_%s", macro->name,
                             pp->macro_invocation_counter, label);

                    char before[TINY16_PP_MAX_LINE_LEN];
                    strncpy(before, expanded, at - expanded);
                    before[at - expanded] = '\0';

                    snprintf(pp->temp_buffer, sizeof(pp->temp_buffer), "%s%s%s", before, unique, p);
                    strcpy(expanded, pp->temp_buffer);
                    at = expanded + strlen(before) + strlen(unique);
                    continue;
                }
            }
            at++;
        }

        fprintf(pp->output, ".file \"%s\"\n.line %zu\n", macro->file, macro->line_numbers[i]);
        fprintf(pp->output, "%s", expanded);
    }

    fprintf(pp->output, ".file \"%s\"\n.line %zu\n", call_file, pp->line);
}

static void tiny16_pp_process_line(Tiny16Preprocessor* pp);

static void tiny16_pp_process_include(Tiny16Preprocessor* pp) {
    const char* p = pp->current_line + 8;
    tiny16_pp_skip_whitespace(&p);

    if (*p != '"') {
        tiny16_pp_set_error(pp, TINY16_PP_ERROR_INVALID_INCLUDE);
        return;
    }

    p++;
    char include_name[TINY16_PP_MAX_LINE_LEN];
    int filename_len = 0;
    while (*p && *p != '"' && filename_len < TINY16_PP_MAX_LINE_LEN - 1) {
        include_name[filename_len++] = *p++;
    }
    include_name[filename_len] = '\0';

    if (*p != '"') {
        tiny16_pp_set_error(pp, TINY16_PP_ERROR_INVALID_INCLUDE);
        return;
    }

    char full_path[TINY16_PP_MAX_LINE_LEN];
    const char* current_file = pp->error_file;
    const char* last_slash = strrchr(current_file, '/');

    if (last_slash) {
        size_t dir_len = last_slash - current_file + 1;
        strncpy(full_path, current_file, dir_len);
        full_path[dir_len] = '\0';
        strcat(full_path, include_name);
    } else {
        strcpy(full_path, include_name);
    }

    for (int i = 0; i < pp->include_depth; i++) {
        if (strcmp(pp->include_stack[i], full_path) == 0) {
            tiny16_pp_set_error(pp, TINY16_PP_ERROR_CIRCULAR_INCLUDE, full_path);
            return;
        }
    }

    if (pp->include_depth >= TINY16_PP_MAX_INCLUDE_DEPTH) {
        tiny16_pp_set_error(pp, TINY16_PP_ERROR_INCLUDE_DEPTH_EXCEEDED,
                            TINY16_PP_MAX_INCLUDE_DEPTH);
        return;
    }

    strcpy(pp->include_stack[pp->include_depth], full_path);
    pp->include_depth++;

    FILE* input = fopen(full_path, "rb");
    if (!input) {
        tiny16_pp_set_error(pp, TINY16_PP_ERROR_FILE_NOT_FOUND, full_path);
        pp->include_depth--;
        return;
    }

    fseek(input, 0, SEEK_END);
    long size = ftell(input);
    fseek(input, 0, SEEK_SET);

    char* included_content = malloc(size + 1);
    if (!included_content) {
        fclose(input);
        pp->include_depth--;
        return;
    }

    fread(included_content, 1, size, input);
    included_content[size] = '\0';
    fclose(input);

    const char* saved_content = pp->content;
    size_t saved_content_len = pp->content_len;
    size_t saved_cursor = pp->cursor;
    size_t saved_line = pp->line;
    char saved_file[TINY16_PP_MAX_LINE_LEN];
    strcpy(saved_file, pp->error_file);

    strcpy(pp->error_file, full_path);
    pp->content = included_content;
    pp->content_len = size;
    pp->cursor = 0;
    pp->line = 1;

    fprintf(pp->output, ".file \"%s\"\n.line 1\n", full_path);

    while (!tiny16_pp_at_end(pp)) {
        tiny16_pp_read_line(pp);
        tiny16_pp_process_line(pp);
        if (tiny16_pp_has_error(pp)) {
            free(included_content);
            strcpy(pp->error_file, saved_file);
            pp->content = saved_content;
            pp->content_len = saved_content_len;
            pp->cursor = saved_cursor;
            pp->line = saved_line;
            pp->include_depth--;
            return;
        }
    }

    if (pp->in_macro_def) {
        tiny16_pp_set_error(pp, TINY16_PP_ERROR_MISMATCHED_ENDMACRO);
        pp->error_line = pp->line;
    }

    free(included_content);
    fprintf(pp->output, ".file \"%s\"\n.line %zu\n", saved_file, saved_line);
    strcpy(pp->error_file, saved_file);
    pp->content = saved_content;
    pp->content_len = saved_content_len;
    pp->cursor = saved_cursor;
    pp->line = saved_line;
    pp->include_depth--;
}

static bool tiny16_pp_line_starts_with(Tiny16Preprocessor* pp, const char* prefix) {
    const char* p = pp->current_line;
    tiny16_pp_skip_whitespace(&p);

    while (*prefix) {
        if (tolower((unsigned char)*p) != tolower((unsigned char)*prefix)) return false;
        p++;
        prefix++;
    }
    return true;
}

static void tiny16_pp_process_line(Tiny16Preprocessor* pp) {
    if (tiny16_pp_line_starts_with(pp, ".macro")) {
        tiny16_pp_parse_macro_header(pp);
        if (tiny16_pp_has_error(pp)) pp->error_line = pp->line;
        return;
    }

    if (tiny16_pp_line_starts_with(pp, ".endmacro")) {
        if (!pp->in_macro_def) {
            tiny16_pp_set_error(pp, TINY16_PP_ERROR_UNEXPECTED_ENDMACRO);
            pp->error_line = pp->line;
        }
        pp->in_macro_def = false;
        pp->current_macro = NULL;
        return;
    }

    if (pp->in_macro_def) {
        if (pp->current_macro->line_count >= TINY16_PP_MAX_MACRO_LINES) {
            tiny16_pp_set_error(pp, TINY16_PP_ERROR_MACRO_TOO_LARGE, TINY16_PP_MAX_MACRO_LINES);
            pp->error_line = pp->line;
            return;
        }
        int idx = pp->current_macro->line_count;
        pp->current_macro->lines[idx] = tiny16_strdup(pp->current_line);
        pp->current_macro->line_numbers[idx] = (pp->line == 0) ? 1 : (pp->line - 1);
        pp->current_macro->line_count++;
        return;
    }

    if (tiny16_pp_line_starts_with(pp, ".include")) {
        tiny16_pp_process_include(pp);
        if (tiny16_pp_has_error(pp)) pp->error_line = pp->line;
        return;
    }

    Tiny16Macro* macro = NULL;
    if (tiny16_pp_is_macro_call(pp, &macro)) {
        tiny16_pp_expand_macro(pp, macro);
        if (tiny16_pp_has_error(pp)) pp->error_line = pp->line;
        return;
    }

    fprintf(pp->output, "%s", pp->current_line);
}

char* tiny16_pp_process_file(Tiny16Preprocessor* pp, const char* filename) {
    FILE* input = fopen(filename, "rb");
    if (!input) {
        tiny16_pp_set_error(pp, TINY16_PP_ERROR_FILE_NOT_FOUND, filename);
        strcpy(pp->error_file, filename);
        return NULL;
    }

    fseek(input, 0, SEEK_END);
    long size = ftell(input);
    fseek(input, 0, SEEK_SET);

    char* content = malloc(size + 1);
    if (!content) {
        fclose(input);
        return NULL;
    }

    fread(content, 1, size, input);
    content[size] = '\0';
    fclose(input);

    FILE* temp = tmpfile();
    if (!temp) {
        free(content);
        return NULL;
    }

    strcpy(pp->error_file, filename);
    pp->output = temp;
    pp->content = content;
    pp->content_len = size;
    pp->cursor = 0;
    pp->line = 1;

    fprintf(pp->output, ".file \"%s\"\n.line 1\n", filename);

    while (!tiny16_pp_at_end(pp)) {
        tiny16_pp_read_line(pp);
        tiny16_pp_process_line(pp);
        if (tiny16_pp_has_error(pp)) {
            free(content);
            fclose(temp);
            return NULL;
        }
    }

    if (pp->in_macro_def) {
        tiny16_pp_set_error(pp, TINY16_PP_ERROR_MISMATCHED_ENDMACRO);
        pp->error_line = pp->line;
        free(content);
        fclose(temp);
        return NULL;
    }

    free(content);

    fseek(temp, 0, SEEK_END);
    long output_size = ftell(temp);
    fseek(temp, 0, SEEK_SET);

    char* output_buffer = malloc(output_size + 1);
    if (!output_buffer) {
        fclose(temp);
        return NULL;
    }

    fread(output_buffer, 1, output_size, temp);
    output_buffer[output_size] = '\0';
    fclose(temp);

    return output_buffer;
}
