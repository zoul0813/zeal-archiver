#include <stdint.h>
#include <zos_vfs.h>
#include <zos_errors.h>

#ifndef ZAR_H
#define ZAR_H

/** Maximum length of the basename (filename without extension) */
#define ZAR_MAX_BASENAME 8

/** Maximum length of the file extension */
#define ZAR_MAX_EXTENSION 3

/** Maximum length of the full filename (basename + extension) */
#define ZAR_MAX_FILENAME (ZAR_MAX_BASENAME + ZAR_MAX_EXTENSION)

/** Maximum number of entries in a ZAR file */
#define ZAR_MAX_ENTRIES ((uint8_t) 255)

/** Value representing an invalid file name */
#define ZAR_INVALID_NAME 0xFF

typedef char zar_filename[ZAR_MAX_FILENAME + 2];

/**
 * @brief Represents an entry in a ZAR file.
 */
typedef struct {
        uint16_t position;
        uint16_t size;
        uint16_t cursor;
} zar_file_entry_t;

/**
 * @brief Represents a ZAR file structure.
 */
typedef struct {
        zos_dev_t fd;
        uint8_t version;
        uint8_t file_count;
} zar_file_t;

/**
 * @brief Opens a ZAR file from a given path.
 */
zos_err_t zar_file_open(const char* path, zar_file_t* zar_file);

/**
 * @brief Closes an open ZAR file.
 */
zos_err_t zar_file_close(zar_file_t* zar_file);

/**
 * @brief Retrieves the contents of a file from a ZAR file entry.
 */
zos_err_t zar_file_read(zar_file_t* zar_file, zar_file_entry_t* entry, uint8_t* buffer, uint16_t* size);

/**
 * @brief Retrieves the file entry from a ZAR file by index.
 */
zos_err_t zar_file_entry_from_index(zar_file_t* zar_file, uint8_t index, zar_file_entry_t *entry);

/**
 * @brief Retrieves the file entry from a ZAR file by filename.
 */
zos_err_t zar_file_entry_from_name(zar_file_t* zar_file, const char* name, zar_file_entry_t *entry);

/**
 * @brief Retrieves the name of a file entry in a ZAR file by index.
 */
zos_err_t zar_file_entry_name_of_index(zar_file_t* zar_file, uint8_t index, zar_filename filename);

/**
 * @brief Retrieves the index of a file entry in a ZAR file by filename.
 */
uint8_t zar_file_entry_index_of_name(zar_file_t* zar_file, const char* name);

#endif // ZAR_H
