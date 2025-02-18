#ifndef ZAR_H
#define ZAR_H


#define ZAR_MAX_BASENAME    8
#define ZAR_MAX_EXTENSION   3
#define ZAR_MAX_FILENAME    (ZAR_MAX_BASENAME + ZAR_MAX_EXTENSION)
#define ZAR_MAX_ENTRIES     ((uint8_t)255)

#define ZAR_INVALID_NAME    0xFF

typedef struct {
    uint16_t position;
    uint16_t size;
    char filename[ZAR_MAX_FILENAME + 2];
} zar_file_entry_t;

typedef struct {
    zos_dev_t fd;
    char header[3];
    uint8_t version;
    uint8_t file_count;
    zar_file_entry_t* entries[ZAR_MAX_ENTRIES];
} zar_file_t;

extern zar_file_entry_t ENTRIES[ZAR_MAX_ENTRIES];

/** Load path as a ZAR File */
zos_err_t zar_file_load(const char *path, zar_file_t *zar_file);
/** Close an open zar_file_t */
zos_err_t zar_file_close(zar_file_t *zar_file);
/** List contents of a zar_file_t */
zos_err_t zar_file_list(zar_file_t *zar_file);
/** Get contents of file from entry */
zos_err_t zar_file_get(zar_file_t *zar_file, zar_file_entry_t *entry);
/** Get contents of file at index */
zos_err_t zar_file_get_from_index(zar_file_t *zar_file, uint8_t index);
/** Get contents of file named by path */
zos_err_t zar_file_get_from_name(zar_file_t *zar_file, const char *name);
/** Get index of file named by path */
uint8_t   zar_file_get_index_of(zar_file_t *zar_file, const char *name);

#endif