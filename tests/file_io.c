#include "zar.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t data[0x10000];
_Alignas(0x4000) static uint8_t buffer[0x8000];
static uint32_t length, cursor;
static unsigned opens, closes, seeks, reads, fail_read, short_read;
static int fail_seek, check_pages;
static zos_dev_t open_result;
static zos_err_t close_result;

static void reset_io(void)
{
    length = sizeof(data);
    cursor = 0;
    opens = closes = seeks = reads = fail_read = short_read = 0;
    fail_seek = check_pages = 0;
    open_result = 3;
    close_result = ERR_SUCCESS;
}

zos_dev_t open(const char* path, int mode)
{
    assert(strcmp(path, "test.zar") == 0 && mode == O_RDONLY);
    opens++;
    return open_result;
}

zos_err_t close(zos_dev_t dev)
{
    assert(dev == 3);
    closes++;
    return close_result;
}

zos_err_t seek(zos_dev_t dev, uint32_t* offset, int whence)
{
    assert(dev == 3 && whence == SEEK_SET);
    seeks++;
    if (fail_seek)
        return ERR_FAILURE;
    if (*offset > length)
        *offset = length;
    cursor = *offset;
    return ERR_SUCCESS;
}

zos_err_t read(zos_dev_t dev, void* buf, uint16_t* size)
{
    assert(dev == 3);
    reads++;
    if (reads == fail_read)
        return ERR_FAILURE;
    if (check_pages)
        assert(*size <= 0x4000 - ((uintptr_t) buf & 0x3fff));
    if (*size > length - cursor)
        *size = length - cursor;
    if (reads == short_read)
        *size /= 2;
    memcpy(buf, data + cursor, *size);
    cursor += *size;
    return ERR_SUCCESS;
}

static void test_open(void)
{
    zar_file_t archive = {9, 8, 7};
    memcpy(data, "ZAR\x01\x02", 5);
    reset_io();
    assert(zar_file_open("test.zar", &archive) == ERR_SUCCESS);
    assert(archive.fd == 3 && archive.version == 1 && archive.file_count == 2);
    assert(opens == 1 && reads == 1 && closes == 0);

    for (unsigned i = 0; i < 3; i++) {
        archive = (zar_file_t) {9, 8, 7};
        data[i] = '!';
        reset_io();
        close_result = ERR_FAILURE; // Preserve the primary error on cleanup failure.
        assert(zar_file_open("test.zar", &archive) == ERR_INVALID_FILESYSTEM);
        assert(closes == 1 && archive.fd == 9);
        memcpy(data, "ZAR", 3);
    }
    for (unsigned i = 0; i < 5; i++) {
        reset_io();
        length = i;
        assert(zar_file_open("test.zar", &archive) == ERR_ENTRY_CORRUPTED);
        assert(closes == 1 && archive.fd == 9);
    }
    reset_io();
    fail_read = 1;
    assert(zar_file_open("test.zar", &archive) == ERR_FAILURE);
    assert(closes == 1 && archive.fd == 9);
    reset_io();
    open_result = -ERR_FAILURE;
    assert(zar_file_open("test.zar", &archive) == ERR_FAILURE);
    assert(reads == 0 && closes == 0);
}

static void test_read(void)
{
    zar_file_t archive = {3, 0, 1};
    zar_file_entry_t entry = {100, 100, 0};
    uint16_t size;
    for (unsigned i = 0; i < sizeof(data); i++)
        data[i] = i % 251;

    reset_io();
    size = 30;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_SUCCESS);
    assert(size == 30 && entry.cursor == 130 && seeks == 1 && reads == 1);
    assert(memcmp(buffer, data + 100, size) == 0);
    size = UINT16_MAX; // This used to require a separate clamp to avoid overflow.
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_SUCCESS);
    assert(size == 70 && entry.cursor == 200);
    assert(memcmp(buffer, data + 130, size) == 0);
    size = 1;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_NO_MORE_ENTRIES);
    assert(size == 0 && seeks == 2 && reads == 2);

    reset_io();
    entry.cursor = 0; // Keep the existing restart convention.
    size = 0;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_SUCCESS);
    assert(size == 0 && seeks == 0 && reads == 0);
    size = 10;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_SUCCESS);
    assert(size == 10 && entry.cursor == 110);

    reset_io();
    entry = (zar_file_entry_t) {65530, 5, 0};
    size = 100;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_SUCCESS);
    assert(size == 5 && entry.cursor == UINT16_MAX);
    size = 1;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_NO_MORE_ENTRIES);
    assert(size == 0);

    reset_io();
    entry = (zar_file_entry_t) {65530, 6, 0};
    size = 100;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_INVALID_OFFSET);
    assert(size == 0 && seeks == 0 && reads == 0);
    entry = (zar_file_entry_t) {65530, 10, 0};
    size = 6; // Otherwise cursor would wrap to zero and restart the entry.
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_INVALID_OFFSET);
    assert(size == 0 && seeks == 0 && reads == 0);
    entry = (zar_file_entry_t) {100, 100, 99};
    size = 100;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_INVALID_OFFSET);
    assert(size == 0 && seeks == 0 && reads == 0);
    entry = (zar_file_entry_t) {100, 100, 201};
    size = 10; // Remaining-byte subtraction must not underflow past EOF.
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_NO_MORE_ENTRIES);
    assert(size == 0 && seeks == 0 && reads == 0);
    entry = (zar_file_entry_t) {100, 0, 0};
    size = 1;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_NO_MORE_ENTRIES);
    assert(size == 0 && seeks == 0 && reads == 0);

    reset_io();
    entry = (zar_file_entry_t) {100, 100, 0};
    fail_seek = 1;
    size = 10;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_FAILURE);
    assert(size == 0 && entry.cursor == 0 && reads == 0);
    reset_io();
    length = 99; // Seek clamps to EOF; the following read detects truncation.
    size = 10;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_ENTRY_CORRUPTED);
    assert(size == 0 && entry.cursor == 100 && reads == 1);

    reset_io();
    length = 105;
    size = 10;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_ENTRY_CORRUPTED);
    assert(size == 5 && entry.cursor == 105);
    reset_io();
    short_read = 1;
    size = 10;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_ENTRY_CORRUPTED);
    assert(size == 5 && entry.cursor == 110);
    reset_io();
    fail_read = 1;
    size = 10;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_FAILURE);
    assert(size == 0 && entry.cursor == 110);

    reset_io();
    entry = (zar_file_entry_t) {100, 0x8000, 0};
    check_pages = 1;
    fail_read = 2;
    size = 0x8000;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_FAILURE);
    assert(size == 0x4000 && entry.cursor == 100 + 0x4000);
    assert(seeks == 1 && reads == 2);
    assert(memcmp(buffer, data + 100, size) == 0);
    fail_read = 0;
    size = 0x8000;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_SUCCESS);
    assert(size == 0x4000 && entry.cursor == 100 + 0x8000);

    reset_io();
    entry.cursor = 0;
    check_pages = 1;
    size = 0x8000;
    assert(zar_file_read(&archive, &entry, buffer, &size) == ERR_SUCCESS);
    assert(size == 0x8000 && entry.cursor == 100 + 0x8000);
    assert(reads == 2 && memcmp(buffer, data + 100, size) == 0);
}

static void test_read_at_and_seek(void)
{
    zar_file_t archive = {3, 0, 1};
    zar_file_entry_t entry = {100, 100, 0};
    struct { uint16_t value; } record;
    uint16_t size;

    reset_io();
    size = sizeof(record);
    assert(zar_file_read_at(&archive, &entry, 10, &record, &size) == ERR_SUCCESS);
    assert(size == sizeof(record) && entry.cursor == 110 + sizeof(record));
    assert(memcmp(&record, data + 110, size) == 0);
    size = sizeof(record);
    assert(zar_file_read(&archive, &entry, &record, &size) == ERR_SUCCESS);
    assert(memcmp(&record, data + 110 + sizeof(record), size) == 0);

    reset_io();
    assert(zar_file_seek(&entry, 0) == ERR_SUCCESS && entry.cursor == 100);
    assert(zar_file_seek(&entry, 25) == ERR_SUCCESS && entry.cursor == 125);
    assert(zar_file_seek(&entry, 100) == ERR_SUCCESS && entry.cursor == 200);
    assert(zar_file_seek(&entry, 101) == ERR_INVALID_OFFSET && entry.cursor == 200);
    size = 1;
    assert(zar_file_read(&archive, &entry, &record, &size) == ERR_NO_MORE_ENTRIES);
    assert(size == 0 && seeks == 0 && reads == 0);
    for (uint16_t offset = 100; offset <= 101; offset++) {
        size = 1;
        assert(zar_file_read_at(&archive, &entry, offset, &record, &size) == ERR_NO_MORE_ENTRIES);
        assert(size == 0 && entry.cursor == 100 + offset && seeks == 0 && reads == 0);
    }
    size = sizeof(record);
    assert(zar_file_read_at(&archive, &entry, 99, &record, &size) == ERR_SUCCESS);
    assert(size == 1 && entry.cursor == 200);

    reset_io();
    entry = (zar_file_entry_t) {65530, 5, 65531};
    assert(zar_file_seek(&entry, 5) == ERR_SUCCESS && entry.cursor == UINT16_MAX);
    for (uint16_t offset = 6; offset <= 7; offset++) {
        size = 1; // Both zero and nonzero wrapped cursors must be rejected.
        assert(zar_file_read_at(&archive, &entry, offset, &record, &size) == ERR_INVALID_OFFSET);
        assert(size == 0 && entry.cursor == UINT16_MAX && seeks == 0 && reads == 0);
        entry.size = 10;
        assert(zar_file_seek(&entry, offset) == ERR_INVALID_OFFSET && entry.cursor == UINT16_MAX);
    }
    entry = (zar_file_entry_t) {100, 0, 0};
    assert(zar_file_seek(&entry, 0) == ERR_SUCCESS && entry.cursor == 100);
    size = 1;
    assert(zar_file_read_at(&archive, &entry, 0, &record, &size) == ERR_NO_MORE_ENTRIES);
    assert(size == 0 && reads == 0 && seeks == 0);
}

int main(void)
{
    test_open();
    test_read();
    test_read_at_and_seek();
    puts("Open cleanup, bounded reads, cursor limits, truncation, and partial I/O passed.");
    return 0;
}
