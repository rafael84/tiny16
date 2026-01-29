#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define TINY16_PP_MAX_MACROS        256
#define TINY16_PP_MAX_MACRO_LINES   256
#define TINY16_PP_MAX_PARAMS        16
#define TINY16_PP_MAX_NAME_LEN      64
#define TINY16_PP_MAX_INCLUDE_DEPTH 16
#define TINY16_PP_MAX_LINE_LEN      1024
#define TINY16_PP_MAX_ERROR_MSG     512

typedef struct {
    char name[TINY16_PP_MAX_NAME_LEN];
    char params[TINY16_PP_MAX_PARAMS][TINY16_PP_MAX_NAME_LEN];
    int param_count;
    char* lines[TINY16_PP_MAX_MACRO_LINES];
    int line_count;
} Tiny16Macro;

typedef enum {
    TINY16_PP_OK = 0,
    TINY16_PP_ERROR_FILE_NOT_FOUND,
    TINY16_PP_ERROR_INCLUDE_DEPTH_EXCEEDED,
    TINY16_PP_ERROR_CIRCULAR_INCLUDE,
    TINY16_PP_ERROR_MACRO_TOO_LARGE,
    TINY16_PP_ERROR_TOO_MANY_MACROS,
    TINY16_PP_ERROR_TOO_MANY_PARAMS,
    TINY16_PP_ERROR_DUPLICATE_MACRO,
    TINY16_PP_ERROR_MISMATCHED_ENDMACRO,
    TINY16_PP_ERROR_UNEXPECTED_ENDMACRO,
    TINY16_PP_ERROR_ARGUMENT_COUNT_MISMATCH,
    TINY16_PP_ERROR_INVALID_INCLUDE,
} Tiny16PPError;

typedef struct {
    Tiny16Macro macros[TINY16_PP_MAX_MACROS];
    int macro_count;
    int macro_invocation_counter;

    char include_stack[TINY16_PP_MAX_INCLUDE_DEPTH][TINY16_PP_MAX_LINE_LEN];
    int include_depth;

    const char* content;
    size_t content_len;
    size_t cursor;
    size_t line;

    FILE* output;

    char temp_args[TINY16_PP_MAX_PARAMS][TINY16_PP_MAX_NAME_LEN];
    int temp_arg_count;
    char current_line[TINY16_PP_MAX_LINE_LEN];
    char temp_buffer[TINY16_PP_MAX_LINE_LEN];

    Tiny16PPError error;
    size_t error_line;
    char error_msg[TINY16_PP_MAX_ERROR_MSG];
    char error_file[TINY16_PP_MAX_LINE_LEN];

    bool in_macro_def;
    Tiny16Macro* current_macro;
} Tiny16Preprocessor;

void tiny16_pp_init(Tiny16Preprocessor* pp);
void tiny16_pp_free(Tiny16Preprocessor* pp);

char* tiny16_pp_process_file(Tiny16Preprocessor* pp, const char* filename);
bool tiny16_pp_has_error(const Tiny16Preprocessor* pp);
void tiny16_pp_print_error(const Tiny16Preprocessor* pp);
