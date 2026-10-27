#include "zar_internal.h"

zos_err_t zar_file_entry_name_of_index(zar_file_t* zar_file, uint8_t index, zar_filename filename)
{
    if (index == ZAR_INVALID_NAME)
        return ERR_INVALID_PATH;
    if (index >= zar_file->file_count)
        return ERR_INVALID_OFFSET;

    uint16_t size;
    zos_err_t err;

    uint32_t offset = ZAR_FILE_HEADER_SIZE + (ZAR_ENTRY_SIZE * index) + sizeof(uint32_t);
    uint32_t pos = offset;
    err = seek(zar_file->fd, &offset, SEEK_SET);
    if (err != ERR_SUCCESS)
        return err;

    if (offset != pos)
        return ERR_INVALID_OFFSET;

    char name[ZAR_MAX_FILENAME];
    size = ZAR_MAX_FILENAME;
    err = read(zar_file->fd, name, &size);
    HANDLE_ERROR(err, size, ZAR_MAX_FILENAME);

    _short_name(name, name + ZAR_MAX_BASENAME, filename);

    return ERR_SUCCESS;
}
