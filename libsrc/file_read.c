#include "zar_internal.h"

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
