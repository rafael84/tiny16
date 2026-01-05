#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "strings.h"

void tiny16_asm_str_strip_comment(char* str) {
    char* cut = strchr(str, ';');
    if (cut) {
        *cut = '\0';
    }
}

void tiny16_asm_str_trim_left(char* str) {
    char* start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

void tiny16_asm_str_trim_right(char* str) {
    if (*str == '\0')
        return;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;
    end[1] = '\0';
}

void tiny16_asm_str_trim(char* str) {
    tiny16_asm_str_trim_left(str);
    tiny16_asm_str_trim_right(str);
}

void tiny16_asm_str_to_upper(char* str) {
    while (*str != '\0') {
        *str = toupper(*str);
        str++;
    }
}

bool tiny16_asm_is_valid_label_prefix(char* str) {
    return (str && (isalpha(str[0]) || str[0] == '_' || str[0] == '.'));
}

int tiny16_asm_label_length(char* str) {
    int len = strlen(str);
    int i;
    if (!(tiny16_asm_is_valid_label_prefix(str))) {
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

long tiny16_asm_str_to_long(const char* str) {
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
