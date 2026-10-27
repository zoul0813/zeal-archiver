#include "zar_internal.h"

zos_err_t zar_file_entry_from_name(zar_file_t* zar_file, const char* name, zar_file_entry_t* entry)
{
    uint8_t index;
    return _find_entry(zar_file, name, &index, entry);
}
