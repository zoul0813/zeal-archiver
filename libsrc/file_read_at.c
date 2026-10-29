#include "zar_internal.h"

zos_err_t zar_file_read_at(zar_file_t* zar_file, zar_file_entry_t* entry, uint16_t offset, void* buffer, uint16_t* size)
{
    // A cursor wrapping to zero would restart the entry in zar_file_read().
    if (offset > UINT16_MAX - entry->position) {
        *size = 0;
        return ERR_INVALID_OFFSET;
    }
    entry->cursor = entry->position + offset;
    return zar_file_read(zar_file, entry, buffer, size);
}
