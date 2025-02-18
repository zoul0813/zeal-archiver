#include <stdarg.h>
#include <zos_vfs.h>
#include "stdutils.h"


uint16_t strlen(const char* str)
{
    uint16_t length = 0;
    while (str[length]) length++;
    return length;
}

void* memset(void* ptr, uint8_t value, size_t size)
{
    uint8_t* p = ptr;
    while (size--) {
        *p++ = value;
    }
    return ptr;
}

void* memcpy(void* dst, const void* src, size_t size)
{
    uint8_t* d       = dst;
    const uint8_t* s = src;
    while (size--) *d++ = *s++;
    return dst;
}

char* strchr(const char* str, uint8_t c)
{
    while (*str) {
        if (*str == (char) c) {
            return (char*) str; // Return pointer to the found character
        }
        str++;
    }

    return NULL; // Return NULL if character not found
}

char* strtok(char* str, const char* delim)
{
    static char* next_token = NULL; // Stores the next position to continue tokenizing
    if (str) {
        next_token = str; // Start with new input string
    } else if (!next_token) {
        return NULL; // No more tokens left
    }

    // Skip leading delimiters
    while (*next_token && strchr(delim, *next_token)) {
        next_token++;
    }

    if (!*next_token)
        return NULL; // If the string is empty after skipping delimiters

    char* token_start = next_token; // Start of the token

    // Find the end of the token
    while (*next_token && !strchr(delim, *next_token)) {
        next_token++;
    }

    if (*next_token) {
        *next_token = '\0'; // Null-terminate the token
        next_token++;       // Move past the delimiter for the next call
    } else {
        next_token = NULL; // No more tokens left
    }

    return token_start;
}

// Helper function to convert an integer to a string
void itoa(int num, char* str, uint8_t base, char alpha) {
    int i = 0;
    int is_negative = 0;

    // Handle 0 explicitly, otherwise empty string is printed
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // Handle negative numbers only if base is 10
    if (num < 0 && base == 10) {
        is_negative = 1;
        num = -num;
    }

    // Process individual digits
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + alpha : rem + '0';
        num = num / base;
    }

    // Append negative sign for negative numbers
    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';

    // Reverse the string
    for (int j = 0, k = i - 1; j < k; j++, k--) {
        char temp = str[j];
        str[j] = str[k];
        str[k] = temp;
    }
}

// Simplified version of printf() with width and alignment support
void __fprintf(zos_dev_t dev, const char* format, va_list args) {
    char buffer[256];  // Output buffer for formatting
    int i = 0;

    while (*format) {
        if (*format == '%' && *(format + 1)) {  // Check for a format specifier
            format++;  // Skip '%' character

            // Parse the width specifier (e.g., %12d or %-5s)
            int width = 0;
            int left_align = 0;

            // Check for negative width (left alignment)
            if (*format == '-') {
                left_align = 1;
                format++;  // Skip the '-' character
            }

            // Parse the width value (e.g., 12 in %12d)
            while (*format >= '0' && *format <= '9') {
                width = width * 10 + (*format - '0');
                format++;
            }

            uint8_t base = 10;
            char alpha = 'a';

            switch (*format) {
                case 's': {  // String
                    const char* str = va_arg(args, const char*);
                    int len = strlen(str);
                    int pad = (width > len) ? width - len : 0;

                    // Left-align if negative width, otherwise right-align
                    if (left_align) {
                        // Copy the string
                        while (*str) {
                            buffer[i++] = *str++;
                        }

                        // Pad with spaces after the string
                        while (pad--) {
                            buffer[i++] = ' ';
                        }
                    } else {
                        // Pad with spaces before the string
                        while (pad--) {
                            buffer[i++] = ' ';
                        }

                        // Copy the string
                        while (*str) {
                            buffer[i++] = *str++;
                        }
                    }
                    break;
                }
                case 'X': alpha = 'A'; // Upper Hex, fallthru
                case 'x': base = 16; // Lower Hex, fallthru
                case 'd': {  // Integer
                    int num = va_arg(args, int);
                    char num_str[20];
                    itoa(num, num_str, base, alpha);  // Convert integer to string
                    int len = strlen(num_str);
                    int pad = (width > len) ? width - len : 0;

                    // Left-align if negative width, otherwise right-align
                    if (left_align) {
                        // Copy the number string
                        char* num_ptr = num_str;
                        while (*num_ptr) {
                            buffer[i++] = *num_ptr++;
                        }

                        // Pad with spaces after the number
                        while (pad--) {
                            buffer[i++] = ' ';
                        }
                    } else {
                        // Pad with spaces before the number
                        while (pad--) {
                            buffer[i++] = ' ';
                        }

                        // Copy the number string
                        char* num_ptr = num_str;
                        while (*num_ptr) {
                            buffer[i++] = *num_ptr++;
                        }
                    }
                    break;
                }
                default:
                    // Handle unknown format specifiers if needed
                    break;
            }
        } else {
            buffer[i++] = *format;  // Copy regular characters
        }
        format++;
    }

    // Null-terminate the buffer
    buffer[i] = '\0';

    // Write the output to the screen using the provided write() function
    uint16_t size = i;
    write(dev, buffer, &size);  // Assume NULL as the device argument
}

void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    __fprintf(DEV_STDOUT, format, args);
    va_end(args);
}

void fprintf(zos_dev_t dev, const char* format, ...) {
    va_list args;
    va_start(args, format);
    __fprintf(dev, format, args);
    va_end(args);
}

