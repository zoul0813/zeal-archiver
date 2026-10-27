#include "zar_internal.h"

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
