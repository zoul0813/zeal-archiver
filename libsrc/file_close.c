#include "zar_internal.h"
#include <stddef.h>

zos_err_t zar_file_close(zar_file_t* zar_file)
{
    zos_err_t err = ERR_INVALID_PARAMETER;
    if (zar_file != NULL) {
        err = close(zar_file->fd);
    }
    return err;
}
