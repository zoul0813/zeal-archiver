#include <zos_vfs.h>

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
