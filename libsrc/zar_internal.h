#ifndef ZAR_INTERNAL_H
#define ZAR_INTERNAL_H

#include <zos_sys.h>
#include "zar.h"

#define ZAR_FILE_HEADER_SIZE 5
#define ZAR_ENTRY_SIZE       (sizeof(uint32_t) + ZAR_MAX_FILENAME)

#define HANDLE_ERROR(error, size, expect)          \
    do {                                           \
        if (error != ERR_SUCCESS) {                \
            return error;                          \
        }                                          \
        if (size != expect) {                      \
            return ERR_ENTRY_CORRUPTED;            \
        }                                          \
    } while (0);

zos_err_t _seek_to_entry_index(zar_file_t* zar_file, uint8_t index);
zos_err_t _find_entry(zar_file_t* zar_file, const char* name, uint8_t* index, zar_file_entry_t* entry);
void _short_name(const char* base, const char* ext, zar_filename filename);
zos_err_t safe_read(zos_dev_t dev, void* buf, uint16_t* size);

#endif // ZAR_INTERNAL_H
