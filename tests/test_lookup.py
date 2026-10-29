"""Exercise archive lookup against an in-memory Zeal VFS: python3 -m unittest discover -s tests."""

from pathlib import Path
import os
import shlex
import subprocess
import tempfile
import unittest


class LookupTest(unittest.TestCase):
    def test_lookup(self):
        self.run_case("lookup", [
            "find_entry", "entry_index_of_name", "entry_from_name",
            "entry_name_of_index", "short_name",
            "entry_from_index", "seek_to_entry_index",
        ])

    def test_file_io(self):
        self.run_case("file_io", [
            "file_open", "entry_read", "entry_read_at", "entry_seek", "safe_read",
            "entry_load", "entry_from_name", "find_entry",
        ])

    def run_case(self, case, sources):
        root = Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            (tmp / "zos_errors.h").write_text("""
#pragma once
typedef enum {
    ERR_SUCCESS, ERR_FAILURE, ERR_INVALID_PATH, ERR_INVALID_OFFSET,
    ERR_ENTRY_CORRUPTED, ERR_INVALID_FILESYSTEM, ERR_NO_MORE_ENTRIES,
    ERR_NO_MORE_MEMORY
} zos_err_t;
""")
            (tmp / "zos_vfs.h").write_text("""
#pragma once
#include <stdint.h>
#include <zos_errors.h>
typedef int8_t zos_dev_t;
enum { SEEK_SET, SEEK_CUR };
#define O_RDONLY 0
zos_dev_t open(const char* path, int mode);
zos_err_t close(zos_dev_t dev);
zos_err_t read(zos_dev_t dev, void* buf, uint16_t* size);
zos_err_t seek(zos_dev_t dev, uint32_t* offset, int whence);
""")
            (tmp / "zos_sys.h").write_text('#include <zos_vfs.h>\n')
            (tmp / "core.h").write_text("""
#include <stddef.h>
int mem_cmp(const void* a, const void* b, size_t size);
""")
            subprocess.run([
                *shlex.split(os.environ.get("CC", "cc")),
                "-std=c11", "-Wall", "-Wextra", "-Werror",
                "-fsanitize=address,undefined", "-g",
                "-I", str(tmp), "-I", str(root / "include"),
                str(root / "tests" / (case + ".c")),
                *(str(root / "libsrc" / (name + ".c")) for name in sources),
                "-o", str(tmp / "lookup"),
            ], check=True)
            subprocess.run([str(tmp / "lookup")], check=True)


if __name__ == "__main__":
    unittest.main()
