#include "zar_internal.h"

zos_err_t zar_file_open(const char* path, zar_file_t* zar_file)
{
    zos_err_t err = ERR_SUCCESS;
    zos_dev_t fd  = open(path, O_RDONLY);
    if (fd < 0) {
        return -fd;
    }

    zar_file->fd = fd;

    // read the header
    uint16_t size = ZAR_FILE_HEADER_SIZE;
    char header[ZAR_FILE_HEADER_SIZE];
    err = read(fd, header, &size);
    HANDLE_ERROR(err, size, ZAR_FILE_HEADER_SIZE);

    if ((header[0] != 'Z') || (header[1] != 'A') | (header[2] != 'R')) {
        return ERR_INVALID_FILESYSTEM;
    }
    zar_file->version    = header[3];
    zar_file->file_count = header[4];

    return err;
}
