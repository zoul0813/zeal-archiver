# Zeal Archiver (ZAR)

Zeal Archiver is a shell utility that can extract and list the contents of ZAR files

ZAR Files are similar to TAR (Tape Archive) files.

The ZAR file format has a 5 byte header containing

### Header

```text
"ZAR" 3-byte file identifier
1 byte file version
1 byte file count
```

### ENTRIES
One entry per file, contiquous

```text
16-bit seek position
16-bit size
MAX_FILENAME filename
```

DATA
----
The data, referenced by seek/size above


## Installation

## Building from source

Make sure that you have [ZDE](https://github.com/zoul0813/zeal-dev-environment) installed.

Then open a terminal, go to the source directory and type the following commands:

```shell
    $ zde make
```

## Using ZAR in a project

In a ZDE project, `ZAR_DIR` should already be set. After your project's Zeal
toolchain setup and `add_executable(game ...)`, find ZAR, link the library, and
use `zar_create` to build the asset archive:

```cmake
find_package(ZAR CONFIG REQUIRED)
target_link_libraries(game PRIVATE zar core)

zar_create(game
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/assets.zar"
)
```

The imported `zar` target supplies the include and library directories.
`zar_create` packs the assets as part of the build and rebuilds the archive when
the input files change. Use an absolute `INPUT` path as above; relative input
paths are resolved from the build directory. The library uses Zeal's `core`
library, which the game must also link.

Put asset files directly in `assets/`, without subdirectories. Use unique 8.3
names such as `TILES.BIN` and `MAP.BIN`; lookup is case-sensitive. ZAR supports
up to 255 entries. Keep the complete archive, including its header and entry
table, within 65,535 bytes so entry end positions fit the library's 16-bit
cursors.

Copy `assets.zar` to the target filesystem alongside the game, or another known
location. Archive creation does not deploy it. The path passed to
`zar_file_open` must point to that file on the Zeal system.

If you are building without ZDE, build ZAR with your Zeal toolchain and set
`ZAR_DIR` to the checkout's `cmake` directory before `find_package`. For example,
with ZAR checked out at `vendor/zeal-archiver`:

```cmake
set(ZAR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/zeal-archiver/cmake")
```

The rest of the CMake integration above is the same.

### Loading a game asset

Include `zar.h`, open the archive, load the entry, and close the archive when
finished. This example loads `TILES.BIN` into a game-owned buffer:

```c
#include <zar.h>

static uint8_t tiles[4096];
static uint16_t tiles_size;

zos_err_t load_tiles(void)
{
    zar_file_t assets;
    zos_err_t err = zar_file_open("assets.zar", &assets);
    if (err != ERR_SUCCESS)
        return err;

    uint16_t size = sizeof(tiles);
    err = zar_entry_load(&assets, "TILES.BIN", tiles, &size);

    zos_err_t close_err = zar_file_close(&assets);
    tiles_size = (err == ERR_SUCCESS) ? size : 0;
    return (err != ERR_SUCCESS) ? err : close_err;
}
```

On input, `size` is the buffer capacity. On return, it is the number of bytes
read. Reset it to the buffer capacity before each load. `zar_entry_load` returns
`ERR_NO_MORE_MEMORY` without changing the buffer if the entry is too large, and
`ERR_INVALID_PATH` if the name is missing or invalid. Empty entries return
`ERR_NO_MORE_ENTRIES`. A truncated payload returns `ERR_ENTRY_CORRUPTED`;
read errors can leave partial data in the buffer, so check the result before
using it. Keep the archive open if the game will load more assets, and close it
on every exit path after a successful open.

### Reading part of an entry

For larger assets, look up an entry once and read only the bytes you need.
This example reads a 16-byte tile at an entry-relative byte offset into a
caller-provided buffer with room for at least 16 bytes:

```c
zos_err_t load_tile(zar_file_t* assets, uint16_t offset, void* tile)
{
    zar_entry_t entry;
    zos_err_t err = zar_entry_from_name(assets, "TILES.BIN", &entry);
    if (err != ERR_SUCCESS)
        return err;

    uint16_t size = 16;
    err = zar_entry_read_at(assets, &entry, offset, tile, &size);
    if (err != ERR_SUCCESS)
        return err;
    return (size == 16) ? ERR_SUCCESS : ERR_ENTRY_CORRUPTED;
}
```

`zar_entry_read_at` sets the entry cursor and advances it by the bytes read.
Subsequent `zar_entry_read` calls continue from that cursor. Reads clamp to the
bytes remaining in the entry; check the returned size when expecting a fixed
record. `zar_entry_seek(&entry, offset)` moves the cursor without reading.
Offsets are relative to the entry start; seeking to its size is allowed, and
the next read returns `ERR_NO_MORE_ENTRIES`.

Read and load buffers accept `void *`, so a struct pointer needs no cast. Only
read directly into a struct when its layout, field sizes, padding, and byte
order match the asset format; otherwise decode the bytes explicitly.

## Deprecated API

Entry functions and the entry type have been renamed. Replace the old names
with these equivalents; arguments, struct layout, and behavior are unchanged:

| Deprecated name | Replacement |
| --- | --- |
| `zar_file_entry_t` | `zar_entry_t` |
| `zar_file_entry_from_name` | `zar_entry_from_name` |
| `zar_file_entry_from_index` | `zar_entry_from_index` |
| `zar_file_entry_name_of_index` | `zar_entry_name_of_index` |
| `zar_file_entry_index_of_name` | `zar_entry_index_of_name` |
| `zar_file_entry_load` | `zar_entry_load` |
| `zar_file_read` | `zar_entry_read` |
| `zar_file_read_at` | `zar_entry_read_at` |
| `zar_file_seek` | `zar_entry_seek` |

`zar.h` automatically includes `deprecated.h`, which maps the old names to the
new ones. Existing source continues to compile without changes. To migrate,
replace the names above and rebuild the library and callers together; these
aliases do not preserve the old linker symbols for precompiled callers.
`zar_file_open`, `zar_file_close`, and `zar_file_t` are unchanged.
