#include "zar_internal.h"

zos_err_t safe_read(zos_dev_t dev, void* buf, uint16_t* size)
{
    uint8_t* ptr       = (uint8_t*) buf;
    uint16_t remaining = *size;
    zos_err_t err      = ERR_SUCCESS;

    while (remaining > 0) {
        uint16_t page_offset    = (uint16_t) ((uintptr_t) ptr & 0x3FFF);
        uint16_t page_remaining = 0x4000 - page_offset;
        uint16_t chunk_size     = (remaining < page_remaining) ? remaining : page_remaining;

        uint16_t this_size = chunk_size;
        err                = read(dev, ptr, &this_size);
        if (err != ERR_SUCCESS) {
            break;
        }

        ptr       += this_size;
        remaining -= this_size;

        if (this_size < chunk_size) {
            // Read operation returned fewer bytes than requested, likely end of stream.
            break;
        }
    }

    *size -= remaining;
    return err;
}
