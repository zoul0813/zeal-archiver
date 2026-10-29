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
 * A zero cursor starts (or restarts) at entry->position. On return, *size is
 * the number of bytes read; the cursor advances even on a partial-read error.
 * EOF returns ERR_NO_MORE_ENTRIES. A short underlying read returns
 * ERR_ENTRY_CORRUPTED. Cursors before the entry start and entry end offsets
 * beyond UINT16_MAX return ERR_INVALID_OFFSET.
 */
zos_err_t zar_file_read(zar_file_t* zar_file, zar_file_entry_t* entry, void* buffer, uint16_t* size);

/**
 * @brief Reads from an entry-relative byte offset, advancing the entry cursor.
 * Uses zar_file_read() semantics, including ERR_NO_MORE_ENTRIES at or past EOF.
 * An offset whose absolute cursor exceeds UINT16_MAX returns ERR_INVALID_OFFSET,
 * sets *size to zero, and leaves the cursor unchanged.
 */
zos_err_t zar_file_read_at(zar_file_t* zar_file, zar_file_entry_t* entry, uint16_t offset, void* buffer, uint16_t* size);

/**
 * @brief Sets the entry cursor to an entry-relative byte offset without I/O.
 * Seeking to entry->size is allowed; the next read returns ERR_NO_MORE_ENTRIES.
 * Offsets past the size or whose absolute cursor exceeds UINT16_MAX return
 * ERR_INVALID_OFFSET and leave the cursor unchanged.
 */
zos_err_t zar_file_seek(zar_file_entry_t* entry, uint16_t offset);

/**
 * @brief Retrieves the file entry from a ZAR file by index.
 */
zos_err_t zar_file_entry_from_index(zar_file_t* zar_file, uint8_t index, zar_file_entry_t *entry);

/**
 * @brief Retrieves the file entry from a ZAR file by filename.
 * Matches the stored zero-padded 8.3 name exactly, including case.
 * Returns ERR_INVALID_PATH for a missing or overlong name, or the seek/read
 * error (ERR_ENTRY_CORRUPTED for a short entry read).
 */
zos_err_t zar_file_entry_from_name(zar_file_t* zar_file, const char* name, zar_file_entry_t *entry);

/**
 * @brief Retrieves the name of a file entry in a ZAR file by index.
 */
zos_err_t zar_file_entry_name_of_index(zar_file_t* zar_file, uint8_t index, zar_filename filename);

/**
 * @brief Retrieves the index of a file entry in a ZAR file by filename.
 * Matches the stored zero-padded 8.3 name exactly, including case.
 * Returns ZAR_INVALID_NAME if the name is missing, invalid, or cannot be read.
 */
uint8_t zar_file_entry_index_of_name(zar_file_t* zar_file, const char* name);

#endif // ZAR_H
