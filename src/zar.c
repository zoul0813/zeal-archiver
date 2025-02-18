#include <stdint.h>
#include <stddef.h>
#include <zos_sys.h>
#include <zos_errors.h>
#include <zos_vfs.h>

#include "zar.h"

#define HANDLE_ERROR(error, size, expect) \
    do {                                  \
        if (error != ERR_SUCCESS)         \
            return error;                 \
        if (size != expect)               \
            return ERR_ENTRY_CORRUPTED;   \
    } while (0);

zar_file_entry_t ENTRIES[ZAR_MAX_ENTRIES];

uint8_t strcmp(const uint8_t* str1, const uint8_t* str2)
{
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *str1 - *str2; // Return difference of the first differing character
}

zos_err_t zar_file_open(const char* path, zar_file_t* zar_file)
{
    zos_err_t err = ERR_SUCCESS;
    zos_dev_t fd  = open(path, O_RDONLY);
    if (fd < 0) {
        return -fd;
    }

    zar_file->fd = fd;

    uint8_t i;
    for (i = 0; i < ZAR_MAX_ENTRIES; i++) {
        zar_file->entries[i] = NULL;
    }

    // read header
    uint16_t size = 3;
    err           = read(fd, &zar_file->header, &size);
    HANDLE_ERROR(err, size, 3);

    size = 1;
    err  = read(fd, &zar_file->version, &size);
    HANDLE_ERROR(err, size, 1);

    size = 1;
    err  = read(fd, &zar_file->file_count, &size);
    HANDLE_ERROR(err, size, 1);

    for (i = 0; i < zar_file->file_count; i++) {
        size                    = 2;
        zar_file_entry_t* entry = &ENTRIES[i];
        entry->cursor           = 0;
        err                     = read(fd, &entry->position, &size);
        HANDLE_ERROR(err, size, 2);

        size = 2;
        err  = read(fd, &entry->size, &size);
        HANDLE_ERROR(err, size, 2);

        char base[ZAR_MAX_BASENAME + 1] = {0x00};
        char ext[ZAR_MAX_EXTENSION + 1] = {0x00};

        size = ZAR_MAX_BASENAME;
        err  = read(fd, &base, &size);
        HANDLE_ERROR(err, size, ZAR_MAX_BASENAME);

        size = ZAR_MAX_EXTENSION;
        err  = read(fd, &ext, &size);
        HANDLE_ERROR(err, size, ZAR_MAX_EXTENSION);

        // sprintf(entry->filename, "%s.%s", base, ext);
        char* dest  = &entry->filename[0];
        uint8_t len = 0;
        while (len < ZAR_MAX_BASENAME && base[len]) {
            dest[len] = base[len];
            len++;
        }
        entry->filename[len] = '.';
        len++;
        dest = &entry->filename[len];
        len  = 0;
        while (len < ZAR_MAX_EXTENSION && ext[len]) {
            dest[len] = ext[len];
            len++;
        }

        zar_file->entries[i] = entry;
    }

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
    read(zar_file->fd, buffer, size);
    if (err != ERR_SUCCESS)
        return err;

    // update the cursor to point to the new read position
    entry->cursor += *size;
    return err;
}

zar_file_entry_t* zar_file_get_from_index(zar_file_t* zar_file, uint8_t index)
{
    if (index == ZAR_INVALID_NAME)
        return NULL;
    if (index >= zar_file->file_count)
        return NULL;
    zar_file_entry_t* entry = zar_file->entries[index];
    return entry;
}

zar_file_entry_t* zar_file_get_from_name(zar_file_t* zar_file, const char* name)
{
    uint8_t index = zar_file_get_index_of(zar_file, name);
    if (index == ZAR_INVALID_NAME)
        return NULL;
    return zar_file_get_from_index(zar_file, index);
}

uint8_t zar_file_get_index_of(zar_file_t* zar_file, const char* name)
{
    if (zar_file == NULL)
        return ZAR_INVALID_NAME;
    uint8_t i;
    for (i = 0; i < zar_file->file_count; i++) {
        zar_file_entry_t* entry = zar_file->entries[i];
        if (strcmp(name, entry->filename) == 0)
            return i;
    }
    return ZAR_INVALID_NAME;
}
