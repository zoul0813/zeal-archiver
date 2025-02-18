#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <zos_sys.h>
#include <zos_errors.h>
#include <zos_vfs.h>

#include "zar.h"

#define HANDLE_ERROR(error,size,expect) \
    do { \
        if(error != ERR_SUCCESS) return error; \
        if(size != expect) return ERR_ENTRY_CORRUPTED; \
    } while(0);

zar_file_entry_t ENTRIES[ZAR_MAX_ENTRIES];

zos_err_t zar_file_load(const char *path, zar_file_t *zar_file) {
    zos_err_t err = ERR_SUCCESS;
    zos_dev_t fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -fd;
    }
    printf("opened: %d\n", fd);

    zar_file->fd = fd;

    uint8_t i;
    for(i = 0; i < ZAR_MAX_ENTRIES; i++) {
        zar_file->entries[i] = NULL;
    }

    // read header
    uint16_t size = 3;
    err = read(fd, &zar_file->header, &size);
    HANDLE_ERROR(err, size, 3);

    size = 1;
    err = read(fd, &zar_file->version, &size);
    HANDLE_ERROR(err, size, 1);

    size = 1;
    err = read(fd, &zar_file->file_count, &size);
    HANDLE_ERROR(err, size, 1);

    for(i = 0; i < zar_file->file_count; i++) {
        size = 2;
        zar_file_entry_t *entry = &ENTRIES[i];
        entry->cursor = 0;
        err = read(fd, &entry->position, &size);
        HANDLE_ERROR(err, size, 2);

        size = 2;
        err = read(fd, &entry->size, &size);
        HANDLE_ERROR(err, size, 2);

        char base[ZAR_MAX_BASENAME + 1] = {0x00};
        char ext[ZAR_MAX_EXTENSION + 1] = {0x00};

        size = ZAR_MAX_BASENAME;
        err = read(fd, &base, &size);
        HANDLE_ERROR(err, size, ZAR_MAX_BASENAME);

        size = ZAR_MAX_EXTENSION;
        err = read(fd, &ext, &size);
        HANDLE_ERROR(err, size, ZAR_MAX_EXTENSION);
        sprintf(entry->filename, "%s.%s", base, ext);

        zar_file->entries[i] = entry;
    }

    return err;
}

zos_err_t zar_file_close(zar_file_t *zar_file) {
    zos_err_t err = ERR_INVALID_PARAMETER;
    if(zar_file != NULL) {
        err = close(zar_file->fd);
    }
    return err;
}

zos_err_t zar_file_list(zar_file_t *zar_file) {
    zos_err_t err = ERR_SUCCESS;

    printf("Filename        Pos   Size\n");
    printf("------------  ----- ------\n");

    uint8_t i;
    for(i = 0; i < zar_file->file_count; i++) {
        zar_file_entry_t *entry = zar_file->entries[i];
        if(entry != NULL) {
            printf("%-12s  %5d %5dB\n", entry->filename, entry->position, entry->size);
        }
    }

    return err;
}

zos_err_t zar_file_read(zar_file_t *zar_file, zar_file_entry_t *entry, uint8_t *buffer, uint16_t *size) {
    zos_err_t err = ERR_SUCCESS;

    if(entry->cursor == 0) {
        entry->cursor = entry->position;
        printf("setting cursor: %d [%d, %d]\n", entry->cursor, entry->position, entry->size);
    }

    // identify the largest cursor position
    uint16_t max_cursor = entry->position + entry->size;
    uint16_t r_size = *size;

    // don't read past the file length
    if(r_size > entry->size) {
        printf("%d exceeds %d, new size(a): ", r_size, entry->size);
        r_size = entry->size;
        printf("%d\n", r_size);
    }

    // prevent reading past the file
    if(entry->cursor >= max_cursor) {
        printf("read past file\n");
        *size = 0;
        return ERR_NO_MORE_ENTRIES; // EOF
    }

    // prevent reading past the file
    if((entry->cursor + r_size - 1) >= max_cursor) {
        printf("%d exceeds %d, new size(b): ", r_size, entry->size);
        r_size = max_cursor - entry->cursor;
        printf("%d\n", r_size);
    }

    *size = r_size;

    // seek to the position within the zar_file
    printf("seeking: %d, ", entry->cursor);
    err = seek(zar_file->fd, &entry->cursor, SEEK_SET);
    if(err != ERR_SUCCESS) return err;
    printf("seeked: %d\n", entry->cursor);

    // read size bytes into the buffer
    read(zar_file->fd, buffer, size);
    printf("read: %d\n", size);
    if(err != ERR_SUCCESS) return err;

    // update the cursor to point to the new read position
    entry->cursor += *size;
    printf("cursor: %d\n", entry->cursor);
    return err;
}

zar_file_entry_t* zar_file_get_from_index(zar_file_t *zar_file, uint8_t index) {
    if(index == ZAR_INVALID_NAME) return NULL;
    if(index >= zar_file->file_count) return NULL;
    zar_file_entry_t *entry = zar_file->entries[index];
    return entry;
}

zar_file_entry_t* zar_file_get_from_name(zar_file_t *zar_file, const char *name) {
    uint8_t index = zar_file_get_index_of(zar_file, name);
    if(index == ZAR_INVALID_NAME) return NULL;
    return zar_file_get_from_index(zar_file, index);
}

uint8_t zar_file_get_index_of(zar_file_t *zar_file, const char *name) {
    if(zar_file == NULL) return ZAR_INVALID_NAME;
    uint8_t i;
    for(i = 0; i < zar_file->file_count; i++) {
        zar_file_entry_t *entry = zar_file->entries[i];
        if(strcmp(name, entry->filename) == 0) return i;
    }
    return ZAR_INVALID_NAME;
}