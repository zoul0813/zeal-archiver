#include "zar_internal.h"

zos_err_t zar_file_seek(zar_file_entry_t* entry, uint16_t offset)
{
    // EOF is allowed, but the public cursor must not wrap.
    if (offset > entry->size || offset > UINT16_MAX - entry->position)
        return ERR_INVALID_OFFSET;
    entry->cursor = entry->position + offset;
    return ERR_SUCCESS;
}
