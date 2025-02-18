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
