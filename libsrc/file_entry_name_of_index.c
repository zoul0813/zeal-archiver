#include "zar_internal.h"

zos_err_t zar_file_entry_name_of_index(zar_file_t* zar_file, uint8_t index, zar_filename filename)
{
    if (index == ZAR_INVALID_NAME)
        return ERR_INVALID_PATH;
    if (index >= zar_file->file_count)
        return ERR_INVALID_OFFSET;

    uint16_t size;
    zos_err_t err;

    err = _seek_to_entry_index(zar_file, index);
    if (err != ERR_SUCCESS)
        return err;

    // skip over the position/size
    uint32_t offset = sizeof(uint32_t);
    uint32_t pos    = offset;
    err             = seek(zar_file->fd, &offset, SEEK_CUR);
    if (err != ERR_SUCCESS)
        return err;

    char base[ZAR_MAX_BASENAME + 1] = {0x00};
    char ext[ZAR_MAX_EXTENSION + 1] = {0x00};

    size = ZAR_MAX_BASENAME;
    err  = read(zar_file->fd, &base, &size);
    HANDLE_ERROR(err, size, ZAR_MAX_BASENAME);

    size = ZAR_MAX_EXTENSION;
    err  = read(zar_file->fd, &ext, &size);
    HANDLE_ERROR(err, size, ZAR_MAX_EXTENSION);

    _short_name(base, ext, filename);

    return ERR_SUCCESS;
}
