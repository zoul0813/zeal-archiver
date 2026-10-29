#include "zar_internal.h"
#include <core.h>
#include <stddef.h>

zos_err_t _find_entry(zar_file_t* zar_file, const char* name, uint8_t* index, zar_entry_t* entry)
{
    uint8_t packed[ZAR_MAX_FILENAME] = {0};
    uint8_t len = 0;
    uint8_t limit = ZAR_MAX_BASENAME;

    // Keep the basename and extension in their separate padded fields.
    while (*name) {
        char c = *name++;
        if (c == '.') {
            if (limit == ZAR_MAX_FILENAME || *name == '\0')
                return ERR_INVALID_PATH;
            len = ZAR_MAX_BASENAME;
            limit = ZAR_MAX_FILENAME;
        } else {
            if (len == limit)
                return ERR_INVALID_PATH;
            packed[len++] = c;
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
