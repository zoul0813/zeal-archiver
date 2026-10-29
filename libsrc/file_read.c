#include "zar_internal.h"

zos_err_t zar_file_read(zar_file_t* zar_file, zar_file_entry_t* entry, void* buffer, uint16_t* size)
{
    uint16_t requested = *size;
    uint16_t cursor = entry->cursor ? entry->cursor : entry->position;
    *size = 0;

    // The public cursor must also represent the EOF position without wrapping.
    uint16_t end = entry->position + entry->size;
    if (end < entry->position || cursor < entry->position)
        return ERR_INVALID_OFFSET;
    if (cursor >= end)
        return ERR_NO_MORE_ENTRIES;

    uint16_t remaining = end - cursor;
    if (requested > remaining)
        requested = remaining;
    if (requested == 0)
        return ERR_SUCCESS;

    uint32_t seek_to = cursor;
    zos_err_t err = seek(zar_file->fd, &seek_to, SEEK_SET);
    if (err != ERR_SUCCESS)
        return err;

    *size = requested;
    err = safe_read(zar_file->fd, buffer, size);
    // A later page read can fail after earlier pages have already been read.
    entry->cursor = cursor + *size;
    if (err == ERR_SUCCESS && *size != requested)
        return ERR_ENTRY_CORRUPTED;
    return err;
}
