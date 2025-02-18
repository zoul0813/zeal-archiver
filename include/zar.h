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

/**
 * @brief Represents an entry in a ZAR file.
 *
 * This structure contains the position, size, and cursor information
 * of a file inside the ZAR file. It also contains the filename
 * associated with the entry.
 */
typedef struct {
        uint16_t position;                   /**< The position of the file entry within the ZAR file */
        uint16_t size;                       /**< The size of the file entry */
        uint16_t cursor;                     /**< Cursor indicating current position within the file */
        char filename[ZAR_MAX_FILENAME + 2]; /**< Filename of the entry (including extension) */
} zar_file_entry_t;

/**
 * @brief Represents a ZAR file structure.
 *
 * This structure contains the file descriptor, header, version,
 * file count, and entries in the ZAR file.
 */
typedef struct {
        zos_dev_t fd;                               /**< File descriptor for the ZAR file */
        char header[3];                             /**< Header information for the ZAR file */
        uint8_t version;                            /**< Version of the ZAR file */
        uint8_t file_count;                         /**< Number of files in the ZAR file */
        zar_file_entry_t* entries[ZAR_MAX_ENTRIES]; /**< Array of pointers to file entries */
} zar_file_t;

/** External array of file entries in the ZAR file */
extern zar_file_entry_t ENTRIES[ZAR_MAX_ENTRIES];

/**
 * @brief Opens a ZAR file from a given path.
 *
 * @param path The path to the ZAR file to open.
 * @param zar_file A pointer to the zar_file_t structure that will hold the opened ZAR file's data.
 * @return An error code, `ERR_SUCCESS` on success.
 */
zos_err_t zar_file_open(const char* path, zar_file_t* zar_file);

/**
 * @brief Closes an open ZAR file.
 *
 * @param zar_file A pointer to the zar_file_t structure representing the open ZAR file.
 * @return An error code, `ERR_SUCCESS` on success.
 */
zos_err_t zar_file_close(zar_file_t* zar_file);

/**
 * @brief Retrieves the contents of a file from a ZAR file entry.
 *
 * @param zar_file  The open zar_file_t.
 * @param entry A pointer to the zar_file_entry_t representing the file entry to read.
 * @param buffer A pointer to the buffer where the file contents will be stored.
 * @param size A pointer to the size of the buffer. Upon return, it contains the number of bytes actually read.
 * @return An error code, `ERR_SUCCESS` on success.
 */
zos_err_t zar_file_read(zar_file_t* zar_file, zar_file_entry_t* entry, uint8_t* buffer, uint16_t* size);

/**
 * @brief Retrieves the file entry from a ZAR file by index.
 *
 * @param zar_file The open zar_file_t.
 * @param index The index of the file entry in the ZAR file.
 * @return A pointer to the zar_file_entry_t corresponding to the given index, or NULL if not found.
 */
zar_file_entry_t* zar_file_get_from_index(zar_file_t* zar_file, uint8_t index);

/**
 * @brief Retrieves the file entry from a ZAR file by filename.
 *
 * @param zar_file The open zar_file_t.
 * @param name The name of the file to retrieve.
 * @return A pointer to the zar_file_entry_t corresponding to the given filename, or NULL if not found.
 */
zar_file_entry_t* zar_file_get_from_name(zar_file_t* zar_file, const char* name);

/**
 * @brief Retrieves the index of a file entry in a ZAR file by filename.
 *
 * @param zar_file The open zar_file_t.
 * @param name The name of the file to find the index for.
 * @return The index of the file in the ZAR file, or `ZAR_INVALID_NAME` if not found.
 */
uint8_t zar_file_get_index_of(zar_file_t* zar_file, const char* name);

#endif // ZAR_H
