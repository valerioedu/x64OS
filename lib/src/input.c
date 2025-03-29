#include "../definitions.h"
#include "../r0io.h"
#include "../../kernel/syscalls/io.h"
#include <stdarg.h>

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static void skip_whitespace(const char** input) {
    while (**input == ' ' || **input == '\t' || **input == '\n') {
        (*input)++;
    }
}

static int str_to_int(const char** input) {
    int result = 0;
    while (is_digit(**input)) {
        result = result * 10 + (**input - '0');
        (*input)++;
    }
    return result;
}

static char input_buffer[1024];
static int buffer_pos = 0;
static int buffer_len = 0;

int scanf(const char* format, ...) {
    if (buffer_pos >= buffer_len) {
        buffer_pos = 0;
        buffer_len = read(0, input_buffer, sizeof(input_buffer) - 1);
        if (buffer_len <= 0) return -1;
        input_buffer[buffer_len] = '\0';
    }
    
    const char* input = input_buffer + buffer_pos;
    
    va_list args;
    va_start(args, format);
    
    int items_matched = 0;
    
    for (const char* ptr = format; *ptr; ptr++) {
        if (*input == '\0') {
            if (*(ptr+1) != '\0') {
                buffer_pos = 0;
                buffer_len = read(0, input_buffer, sizeof(input_buffer) - 1);
                if (buffer_len <= 0) goto end_scanning;
                input_buffer[buffer_len] = '\0';
                input = input_buffer;
            } else {
                goto end_scanning;
            }
        }
        
        if (*ptr == '%') {
            ptr++;
            
            switch (*ptr) {
                case 'd': {
                    skip_whitespace(&input);
                    if (!is_digit(*input) && *input != '-' && *input != '+')
                        goto end_scanning;
                        
                    int negative = 0;
                    if (*input == '-') {
                        negative = 1;
                        input++;
                    } else if (*input == '+') {
                        input++;
                    }
                    
                    int value = str_to_int(&input);
                    if (negative) value = -value;
                    
                    int* out_val = va_arg(args, int*);
                    *out_val = value;
                    items_matched++;
                    break;
                }
                case 's': {
                    skip_whitespace(&input);
                    if (*input == '\0')
                        goto end_scanning;
                        
                    char* out_str = va_arg(args, char*);
                    int i = 0;
                    
                    while (*input && *input != ' ' && *input != '\n' && *input != '\t') {
                        out_str[i++] = *input++;
                    }
                    out_str[i] = '\0';
                    items_matched++;
                    break;
                }
                case 'c': {
                    char* out_char = va_arg(args, char*);
                    *out_char = *input++;
                    items_matched++;
                    break;
                }
                case '%':
                    if (*input == '%') {
                        input++;
                    } else {
                        goto end_scanning;
                    }
                    break;
                default:
                    goto end_scanning;
            }
        } else if (*ptr == ' ' || *ptr == '\t' || *ptr == '\n') {
            skip_whitespace(&input);
        } else {
            if (*input == *ptr) {
                input++;
            } else {
                goto end_scanning;
            }
        }
    }
    
end_scanning:
    buffer_pos = input - input_buffer;
    va_end(args);
    return items_matched;
}

char* kfgets(char* buffer, int size, int fd) {
    if (buffer == NULL || size <= 0) {
        return NULL;
    }

    int bytes_read = read(fd, buffer, size - 1);
    
    if (bytes_read <= 0) {
        return NULL;
    }
    
    buffer[bytes_read] = '\0';
    
    for (int i = 0; i < bytes_read; i++) {
        if (buffer[i] == '\n') {
            buffer[i] = '\0';
            break;
        }
    }
    
    return buffer;
}
