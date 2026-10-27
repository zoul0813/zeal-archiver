#include "zar_internal.h"
#include <core.h>

uint8_t zar_file_entry_index_of_name(zar_file_t* zar_file, const char* name)
{
    uint8_t i;
    zar_filename filename;

    for (i = 0; i < zar_file->file_count; i++) {
        zar_file_entry_name_of_index(zar_file, i, filename);

        if (str_cmp(filename, name) == 0) {
            return i;
        }
    }
    return ZAR_INVALID_NAME;
}
