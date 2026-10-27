#include "zar_internal.h"

void _short_name(const char* base, const char* ext, zar_filename filename)
{
    char* dest  = filename;
    uint8_t len = 0;
    while (len < ZAR_MAX_BASENAME && base[len]) {
        dest[len] = base[len];
        len++;
    }

    if (ext[0] == '\0') {
        dest[len] = '\0';
        return;
    }

    dest[len] = '.';
    len++;
    dest = &filename[len];
    len  = 0;
    while (len < ZAR_MAX_EXTENSION && ext[len]) {
        dest[len] = ext[len];
        len++;
    }
    dest[len] = '\0'; // null terminate
}
