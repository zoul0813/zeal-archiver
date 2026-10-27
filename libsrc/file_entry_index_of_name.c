#include "zar_internal.h"
#include <stddef.h>

uint8_t zar_file_entry_index_of_name(zar_file_t* zar_file, const char* name)
{
    uint8_t index = ZAR_INVALID_NAME;
    _find_entry(zar_file, name, &index, NULL);
    return index;
}
