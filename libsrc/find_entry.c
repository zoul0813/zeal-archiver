#include "zar_internal.h"
#include <core.h>
#include <stddef.h>

zos_err_t _find_entry(zar_file_t* zar_file, const char* name, uint8_t* index, zar_file_entry_t* entry)
{
    uint8_t packed[ZAR_MAX_FILENAME] = {0};
    uint8_t len = 0;

    // Match the displayed 8.3 name exactly; never truncate or change case.
    while (*name && *name != '.') {
        if (len == ZAR_MAX_BASENAME)
            return ERR_INVALID_PATH;
        packed[len++] = *name++;
    }
    if (*name == '.') {
        name++;
        if (*name == '\0')
            return ERR_INVALID_PATH;
        len = ZAR_MAX_BASENAME;
        while (*name) {
            if (len == ZAR_MAX_FILENAME || *name == '.')
                return ERR_INVALID_PATH;
            packed[len++] = *name++;
        }
    }

    if (zar_file->file_count == 0)
        return ERR_INVALID_PATH;

    uint32_t offset = ZAR_FILE_HEADER_SIZE;
    zos_err_t err = seek(zar_file->fd, &offset, SEEK_SET);
    if (err != ERR_SUCCESS)
        return err;
    if (offset != ZAR_FILE_HEADER_SIZE)
        return ERR_INVALID_OFFSET;

    uint8_t record[ZAR_ENTRY_SIZE];
    uint8_t i;
    for (i = 0; i < zar_file->file_count; i++) {
        uint16_t size = ZAR_ENTRY_SIZE;
        err = read(zar_file->fd, record, &size);
        HANDLE_ERROR(err, size, ZAR_ENTRY_SIZE);

        if (mem_cmp(record + sizeof(uint32_t), packed, ZAR_MAX_FILENAME) == 0) {
            *index = i;
            if (entry != NULL) {
                entry->position = (uint16_t) record[0] | ((uint16_t) record[1] << 8);
                entry->size = (uint16_t) record[2] | ((uint16_t) record[3] << 8);
                entry->cursor = 0;
            }
            return ERR_SUCCESS;
        }
    }
    return ERR_INVALID_PATH;
}
