#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <zos_vfs.h>

/**
 * @brief Calculates the length of a null-terminated string.
 *
 * @param str A pointer to the null-terminated string to be measured.
 * @return The length of the string (excluding the null terminator).
 */
uint16_t strlen(const char* str);

/**
 * @brief Fills a block of memory with a specific value.
 *
 * @param ptr A pointer to the memory block to be filled.
 * @param value The value to set each byte in the memory block to.
 * @param size The number of bytes to fill in the memory block.
 * @return A pointer to the memory block `ptr` after filling.
 */
void* memset(void* ptr, uint8_t value, size_t size);

/**
 * @brief Copies a block of memory from a source to a destination.
 *
 * @param dst A pointer to the destination memory block where the data will be copied.
 * @param src A pointer to the source memory block from which data is copied.
 * @param size The number of bytes to copy.
 * @return A pointer to the destination memory block `dst`.
 */
void* memcpy(void* dst, const void* src, size_t size);

/**
 * @brief Searches for the first occurrence of a character in a string.
 *
 * @param str A pointer to the null-terminated string to be searched.
 * @param c The character to search for.
 * @return A pointer to the first occurrence of the character `c` in the string `str`.
 *         If the character is not found, `NULL` is returned.
 */
char* strchr(const char* str, uint8_t c);

/**
 * @brief Splits a string into tokens based on delimiters.
 *
 * @param str A pointer to the string to be tokenized. On the first call, this should
 *            be the full string. On subsequent calls, it should be `NULL`.
 * @param delim A string containing the delimiters that separate tokens.
 * @return A pointer to the next token found in the string, or `NULL` if no more tokens are available.
 */
char* strtok(char* str, const char* delim);

/**
 * @brief Converts an integer `num` to a string `str` representation in the specified `base`.
 *
 * @param num The integer to convert.
 * @param str A buffer to store the resulting string.
 * @param base The base to use for conversion (e.g., 10 for decimal, 16 for hexadecimal).
 * @param alpha A character indicating whether hexadecimal digits should be in lowercase
 *              ('a') or uppercase ('A').
 */
void itoa(int num, char* str, uint8_t base, char alpha);

/**
 * @brief Formats and prints output to the standard output (typically the screen).
 *
 * @param format A string that specifies how subsequent arguments are formatted and displayed.
 *               Supports various format specifiers.
 * @param ... A variable list of arguments that will be formatted according to the format string.
 */
void printf(const char* format, ...);

/**
 * @brief Formats and prints output to a specified device (e.g., screen or file).
 *
 * @param dev The device where the output will be written (e.g., `DEV_STDOUT` for the screen).
 * @param format A string that specifies how subsequent arguments are formatted and displayed.
 *               Supports various format specifiers.
 * @param ... A variable list of arguments that will be formatted according to the format string.
 */
void fprintf(zos_dev_t dev, const char* format, ...);