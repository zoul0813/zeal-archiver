#include "zar_internal.h"

zos_err_t zar_file_entry_from_name(zar_file_t* zar_file, const char* name, zar_file_entry_t* entry)
{
    uint8_t index = zar_file_entry_index_of_name(zar_file, name);
    if (index == ZAR_INVALID_NAME)
        return ERR_INVALID_PATH;
    return zar_file_entry_from_index(zar_file, index, entry);
}
