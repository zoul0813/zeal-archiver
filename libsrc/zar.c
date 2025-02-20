#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <zos_sys.h>
#include <zos_errors.h>
#include <zos_vfs.h>

#include "zar.h"

#define ZAR_FILE_HEADER_SIZE 5
#define ZAR_ENTRY_SIZE       (sizeof(uint32_t) + ZAR_MAX_FILENAME)

#define HANDLE_ERROR(error, size, expect)          \
    do {                                           \
        if (error != ERR_SUCCESS) {                \
            printf("ERROR: line: %d\n", __LINE__); \
            return error;                          \
        }                                          \
        if (size != expect) {                      \
            printf("ERROR: line: %d\n", __LINE__); \
            return ERR_ENTRY_CORRUPTED;            \
        }                                          \
    } while (0);

/** INTERNALS **/

//
uint8_t strcmp(const uint8_t* str1, const uint8_t* str2)
{
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *str1 - *str2; // Return difference of the first differing character
}

zos_err_t _seek_to_entry_index(zar_file_t* zar_file, uint8_t index)
{
    // seek to index
    zos_err_t err;
    uint32_t cursor = ZAR_FILE_HEADER_SIZE + (ZAR_ENTRY_SIZE * index);
    uint32_t pos    = cursor;
    err             = seek(zar_file->fd, &cursor, SEEK_SET);
    if (err != ERR_SUCCESS)
        return err;

    if (pos != cursor)
        return ERR_INVALID_OFFSET;
    return ERR_SUCCESS;
}

void _short_name(const char* base, const char* ext, zar_filename filename)
{
    char* dest  = filename;
    uint8_t len = 0;
    while (len < ZAR_MAX_BASENAME && base[len]) {
        dest[len] = base[len];
        len++;
    }

    if (ext[0] == '\0') {
        dest[len] = '\0';
        return;
    }

    dest[len] = '.';
    len++;
    dest = &filename[len];
    len  = 0;
    while (len < ZAR_MAX_EXTENSION && ext[len]) {
        dest[len] = ext[len];
        len++;
    }
    dest[len] = '\0'; // null terminate
}

zos_err_t safe_read(zos_dev_t dev, void* buf, uint16_t* size)
{
    uint8_t* ptr       = (uint8_t*) buf;
    uint16_t remaining = *size;
    zos_err_t err      = ERR_SUCCESS;

    while (remaining > 0) {
        uint16_t page_offset    = (uint16_t) ((uintptr_t) ptr & 0x3FFF);
        uint16_t page_remaining = 0x4000 - page_offset;
        uint16_t chunk_size     = (remaining < page_remaining) ? remaining : page_remaining;

        uint16_t this_size = chunk_size;
        err                = read(dev, ptr, &this_size);
        if (err != ERR_SUCCESS) {
            break;
        }

        ptr       += this_size;
        remaining -= this_size;

        if (this_size < chunk_size) {
            // Read operation returned fewer bytes than requested, likely end of stream.
            break;
        }
    }

    *size -= remaining;
    return err;
}

/** ZAR Library **/

//
zos_err_t zar_file_open(const char* path, zar_file_t* zar_file)
{
    zos_err_t err = ERR_SUCCESS;
    zos_dev_t fd  = open(path, O_RDONLY);
    if (fd < 0) {
        return -fd;
    }

    zar_file->fd = fd;

    // read the header
    uint16_t size = ZAR_FILE_HEADER_SIZE;
    char header[ZAR_FILE_HEADER_SIZE];
    err = read(fd, header, &size);
    HANDLE_ERROR(err, size, ZAR_FILE_HEADER_SIZE);

    if ((header[0] != 'Z') || (header[1] != 'A') | (header[2] != 'R')) {
        return ERR_INVALID_FILESYSTEM;
    }
    zar_file->version    = header[3];
    zar_file->file_count = header[4];

    return err;
}

zos_err_t zar_file_close(zar_file_t* zar_file)
{
    zos_err_t err = ERR_INVALID_PARAMETER;
    if (zar_file != NULL) {
        err = close(zar_file->fd);
    }
    return err;
}

zos_err_t zar_file_read(zar_file_t* zar_file, zar_file_entry_t* entry, uint8_t* buffer, uint16_t* size)
{
    zos_err_t err = ERR_SUCCESS;

    if (entry->cursor == 0) {
        entry->cursor = entry->position;
    }

    // identify the largest cursor position
    uint16_t max_cursor = entry->position + entry->size;
    uint16_t r_size     = *size;

    // don't read past the file length
    if (r_size > entry->size) {
        r_size = entry->size;
    }

    // prevent reading past the file
    if (entry->cursor >= max_cursor) {
        *size = 0;
        return ERR_NO_MORE_ENTRIES; // EOF
    }

    // prevent reading past the file
    if ((entry->cursor + r_size - 1) >= max_cursor) {
        r_size = max_cursor - entry->cursor;
    }

    *size            = r_size;
    uint32_t seek_to = entry->cursor;

    // seek to the position within the zar_file
    err = seek(zar_file->fd, &seek_to, SEEK_SET);
    if (err != ERR_SUCCESS)
        return err;
    entry->cursor = (uint16_t) seek_to;

    // read size bytes into the buffer
    err = safe_read(zar_file->fd, buffer, size);
    if (err != ERR_SUCCESS)
        return err;

    // update the cursor to point to the new read position
    entry->cursor += *size;
    return err;
}

zos_err_t zar_file_entry_from_index(zar_file_t* zar_file, uint8_t index, zar_file_entry_t* entry)
{
    if (index == ZAR_INVALID_NAME)
        return NULL;
    if (index >= zar_file->file_count)
        return NULL;

    uint16_t size;
    zos_err_t err;

    err = _seek_to_entry_index(zar_file, index);
    if (err != ERR_SUCCESS)
        return err;

    // read position + size
    size = sizeof(uint32_t);
    err  = read(zar_file->fd, entry, &size);
    HANDLE_ERROR(err, size, sizeof(uint32_t));

    return ERR_SUCCESS;
}

zos_err_t zar_file_entry_from_name(zar_file_t* zar_file, const char* name, zar_file_entry_t* entry)
{
    uint8_t index = zar_file_entry_index_of_name(zar_file, name);
    if (index == ZAR_INVALID_NAME)
        return NULL;
    return zar_file_entry_from_index(zar_file, index, entry);
}

zos_err_t zar_file_entry_name_of_index(zar_file_t* zar_file, uint8_t index, zar_filename filename)
{
    if (index == ZAR_INVALID_NAME)
        return NULL;
    if (index >= zar_file->file_count)
        return NULL;

    uint16_t size;
    zos_err_t err;

    err = _seek_to_entry_index(zar_file, index);
    if (err != ERR_SUCCESS)
        return err;

    // skip over the position/size
    uint32_t offset = sizeof(uint32_t);
    uint32_t pos    = offset;
    err             = seek(zar_file->fd, &offset, SEEK_CUR);
    if (err != ERR_SUCCESS)
        return err;

    char base[ZAR_MAX_BASENAME + 1] = {0x00};
    char ext[ZAR_MAX_EXTENSION + 1] = {0x00};

    size = ZAR_MAX_BASENAME;
    err  = read(zar_file->fd, &base, &size);
    HANDLE_ERROR(err, size, ZAR_MAX_BASENAME);

    size = ZAR_MAX_EXTENSION;
    err  = read(zar_file->fd, &ext, &size);
    HANDLE_ERROR(err, size, ZAR_MAX_EXTENSION);

    _short_name(base, ext, filename);

    return ERR_SUCCESS;
}

uint8_t zar_file_entry_index_of_name(zar_file_t* zar_file, const char* name)
{
    zos_err_t err;
    uint16_t size;
    uint8_t i;
    zar_filename filename;

    for (i = 0; i < zar_file->file_count; i++) {
        zar_file_entry_name_of_index(zar_file, i, filename);

        if (strcmp(filename, name) == 0) {
            return i;
        }
    }
    return ZAR_INVALID_NAME;
}
