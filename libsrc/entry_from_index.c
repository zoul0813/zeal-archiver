#include "zar_internal.h"

zos_err_t zar_entry_from_index(zar_file_t* zar_file, uint8_t index, zar_entry_t* entry)
{
    if (index == ZAR_INVALID_NAME)
        return ERR_INVALID_PATH;
    if (index >= zar_file->file_count)
        return ERR_INVALID_OFFSET;

    uint16_t size;
    zos_err_t err;

    err = _seek_to_entry_index(zar_file, index);
    if (err != ERR_SUCCESS)
        return err;

    // read position + size
    size = sizeof(uint32_t);
    err  = read(zar_file->fd, entry, &size);
    HANDLE_ERROR(err, size, sizeof(uint32_t));

    entry->cursor = 0;

    return ERR_SUCCESS;
}
