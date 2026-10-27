#include "zar.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t data[5 + 15 * 255];
static uint32_t cursor;
static unsigned seeks, reads, fail_read;
static int fail_seek, wrong_seek, short_read;
static zar_file_t archive = {0, 0, 5};

static void reset_io(void)
{
    cursor = 123;
    seeks = reads = fail_read = 0;
    fail_seek = wrong_seek = short_read = 0;
}

zos_err_t seek(zos_dev_t dev, uint32_t* offset, int whence)
{
    (void) dev;
    assert(whence == SEEK_SET);
    seeks++;
    if (fail_seek)
        return ERR_FAILURE;
    if (wrong_seek)
        (*offset)++;
    cursor = *offset;
    return ERR_SUCCESS;
}

zos_err_t read(zos_dev_t dev, void* buf, uint16_t* size)
{
    (void) dev;
    reads++;
    if (reads == fail_read)
        return ERR_FAILURE;
    if (short_read)
        (*size)--;
    assert(cursor + *size <= sizeof(data));
    memcpy(buf, data + cursor, *size);
    cursor += *size;
    return ERR_SUCCESS;
}

int mem_cmp(const void* a, const void* b, size_t size)
{
    return memcmp(a, b, size);
}

static void record(unsigned i, const char* base, const char* ext)
{
    uint8_t* p = data + 5 + 15 * i;
    memset(p, 0, 15);
    p[0] = 0x23;
    p[1] = 0xf1;
    p[2] = 0xcd;
    p[3] = 0xab;
    memcpy(p + 4, base, strlen(base));
    memcpy(p + 12, ext, strlen(ext));
}

int main(void)
{
    const char* names[] = {"HELLO.TXT", "README", "abcdefgh.xyz", ".TXT", ""};
    record(0, "HELLO", "TXT");
    record(1, "README", "");
    record(2, "abcdefgh", "xyz");
    record(3, "", "TXT");
    record(4, "", "");

    for (uint8_t i = 0; i < 5; i++) {
        reset_io();
        assert(zar_file_entry_index_of_name(&archive, names[i]) == i);
        assert(seeks == 1 && reads == (unsigned) i + 1);

        zar_file_entry_t entry = {1, 2, 3};
        reset_io();
        assert(zar_file_entry_from_name(&archive, names[i], &entry) == ERR_SUCCESS);
        assert(entry.position == 0xf123 && entry.size == 0xabcd && entry.cursor == 0);
        assert(seeks == 1 && reads == (unsigned) i + 1);

        zar_filename filename;
        reset_io();
        assert(zar_file_entry_name_of_index(&archive, i, filename) == ERR_SUCCESS);
        assert(strcmp(filename, names[i]) == 0);
        assert(seeks == 1 && reads == 1);
    }

    const char* missing[] = {
        "hello.TXT", "HELLO.txt", "HELLO", "MISSING.TXT", "README.",
        "abcdefghi.xyz", "abcdefgh.xyzz", "HELLO.TXT.more", "HELLO..TXT",
    };
    for (unsigned i = 0; i < sizeof(missing) / sizeof(*missing); i++) {
        zar_file_entry_t entry = {1, 2, 3};
        reset_io();
        assert(zar_file_entry_index_of_name(&archive, missing[i]) == ZAR_INVALID_NAME);
        assert(zar_file_entry_from_name(&archive, missing[i], &entry) == ERR_INVALID_PATH);
        assert(entry.position == 1 && entry.size == 2 && entry.cursor == 3);
    }

    zar_file_entry_t entry = {1, 2, 3};
    zar_filename filename;
    reset_io();
    fail_seek = 1;
    assert(zar_file_entry_from_name(&archive, "HELLO.TXT", &entry) == ERR_FAILURE);
    assert(zar_file_entry_index_of_name(&archive, "HELLO.TXT") == ZAR_INVALID_NAME);
    assert(zar_file_entry_name_of_index(&archive, 0, filename) == ERR_FAILURE);
    assert(reads == 0);

    reset_io();
    wrong_seek = 1;
    assert(zar_file_entry_from_name(&archive, "HELLO.TXT", &entry) == ERR_INVALID_OFFSET);
    assert(zar_file_entry_name_of_index(&archive, 0, filename) == ERR_INVALID_OFFSET);
    assert(reads == 0);

    reset_io();
    fail_read = 2;
    assert(zar_file_entry_from_name(&archive, "README", &entry) == ERR_FAILURE);
    assert(seeks == 1 && reads == 2);
    reset_io();
    fail_read = 1;
    assert(zar_file_entry_index_of_name(&archive, "HELLO.TXT") == ZAR_INVALID_NAME);
    reset_io();
    fail_read = 1;
    assert(zar_file_entry_name_of_index(&archive, 0, filename) == ERR_FAILURE);

    reset_io();
    short_read = 1;
    assert(zar_file_entry_from_name(&archive, "HELLO.TXT", &entry) == ERR_ENTRY_CORRUPTED);
    assert(zar_file_entry_index_of_name(&archive, "HELLO.TXT") == ZAR_INVALID_NAME);
    assert(zar_file_entry_name_of_index(&archive, 0, filename) == ERR_ENTRY_CORRUPTED);
    assert(entry.position == 1 && entry.size == 2 && entry.cursor == 3);

    reset_io();
    assert(zar_file_entry_name_of_index(&archive, ZAR_INVALID_NAME, filename) == ERR_INVALID_PATH);
    assert(zar_file_entry_name_of_index(&archive, 5, filename) == ERR_INVALID_OFFSET);
    assert(seeks == 0 && reads == 0);

    archive.file_count = 0;
    reset_io();
    assert(zar_file_entry_index_of_name(&archive, "HELLO.TXT") == ZAR_INVALID_NAME);
    assert(zar_file_entry_from_name(&archive, "HELLO.TXT", &entry) == ERR_INVALID_PATH);
    assert(seeks == 0 && reads == 0);

    archive.file_count = 255;
    for (unsigned i = 0; i < 255; i++) {
        char base[9];
        snprintf(base, sizeof(base), "F%07u", i);
        record(i, base, "BIN");
    }
    reset_io();
    assert(zar_file_entry_index_of_name(&archive, "F0000254.BIN") == 254);
    assert(seeks == 1 && reads == 255);
    reset_io();
    assert(zar_file_entry_index_of_name(&archive, "MISSING.BIN") == ZAR_INVALID_NAME);
    assert(seeks == 1 && reads == 255);
    puts("Lookup, listing, syscall counts, error handling, and 255-entry scan passed.");
    return 0;
}
