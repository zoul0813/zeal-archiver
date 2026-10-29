#include "zar_internal.h"

zos_err_t zar_entry_load(zar_file_t* zar_file, const char* name, void* buffer, uint16_t* size)
{
    zar_entry_t entry;
    uint16_t capacity = *size;
    *size = 0;

    zos_err_t err = zar_entry_from_name(zar_file, name, &entry);
    if (err != ERR_SUCCESS)
        return err;
    // Reject insufficient capacity rather than returning a truncated entry.
    if (entry.size > capacity)
        return ERR_NO_MORE_MEMORY;

    *size = entry.size;
    return zar_entry_read(zar_file, &entry, buffer, size);
}
