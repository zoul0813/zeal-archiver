#!/usr/bin/env python3

import argparse
import os
import re
import struct
from pathlib import Path

parser = argparse.ArgumentParser("gif2zeal")
parser.add_argument("-i", "--input", help="Input Folder", required=True)
parser.add_argument("-o", "--output", help="Output File", required=False)
parser.add_argument("-v", "--verbose", help="Verbose output", action='store_true')
parser.add_argument("-x", "--extract", help="Extract Input to Output", action='store_true')
parser.add_argument("-l", "--list", help="List archive files", action='store_true')

MAX_ENTRIES      = 255
MAX_BASENAME     = 8
MAX_EXTENSION    = 3
MAX_FILENAME     = MAX_BASENAME + MAX_EXTENSION
FILE_HEADER_SIZE = 4 + MAX_FILENAME + MAX_EXTENSION # uint16_t, uint16_t, char[8]

def zar_to_os(filename):
  return filename[:MAX_BASENAME].rstrip('\x00') + "." + filename[MAX_BASENAME:].rstrip('\x00')

def generate_zar_filenames(filenames):
  seen = {}
  normalized = {}

  for name in filenames:
    # Remove non-alphanumeric characters (optional)
    base, ext = os.path.splitext(os.path.basename(name))
    base = re.sub(r'[^a-zA-Z0-9]', '', base).ljust(MAX_BASENAME, '\x00')
    ext = re.sub(r'[^a-zA-Z0-9]', '', ext)

    base_name = base[:MAX_BASENAME] + ext[:MAX_EXTENSION]
    count = seen.get(base_name, 0)
    if count > 0:
      short_name = short_name[:MAX_BASENAME - 3] + str(count).zfill(3) + ext[:MAX_EXTENSION]
    else:
      short_name = base_name
    seen[base_name] = count + 1

    short_name = short_name.ljust(MAX_FILENAME, '\x00')
    normalized[name] = short_name
  return normalized

def archive(args):
  if not args.output:
    print("Output filename required")
    return

  print("Archiving ", args.input, "to", args.output)
  # get absolute paths
  rel_names = sorted(os.listdir(args.input))
  # filter out hidden dot files
  filenames = [os.path.abspath(os.path.join(args.input, file)) for file in rel_names if not os.path.basename(file).startswith(".")]

  file_count = len(filenames)
  if file_count > MAX_ENTRIES:
    print("ZAR has a", MAX_ENTRIES, "file limit")
    return

  header = "ZAR"
  version = 0

  total_size = 0
  pointer = 0

  with open(args.output, "wb") as output:
    total_size += output.write(header.encode("ascii"))
    total_size += output.write(struct.pack("B", version))
    total_size += output.write(struct.pack("B", file_count))

    zar_files = generate_zar_filenames(filenames)

    pointer = total_size + (FILE_HEADER_SIZE * len(zar_files))
    for src, short_name in zar_files.items():
      size = os.path.getsize(src)
      short = short_name.encode("ascii")

      # start position of file
      total_size += output.write(struct.pack("<H", pointer))
      # size of the file
      total_size += output.write(struct.pack("<H", size))
      total_size += output.write(short)

      if args.verbose:
        print(pointer, size, short)

      # add the filesize to pointer
      pointer += size

    for src, short_name in zar_files.items():
      with open(src, "rb") as input:
        total_size += output.write(input.read())

def extract(args):
  if not args.list and not args.output:
    print("Output folder required")
    return

  print("Extracting ", args.input, "to", args.output)
  if not args.list:
    os.makedirs(args.output, exist_ok=True)

  with open(args.input, "rb") as input:
    header = input.read(3).decode("ascii")
    version = ord(input.read(1))
    file_count = ord(input.read(1))

    print(header, version, file_count)

    files = {}

    bytes_read = 0
    for i in range(file_count):
      data = input.read(2)
      pointer = struct.unpack("<H", data)[0]
      data = input.read(2)
      size = struct.unpack("<H", data)[0]
      data = input.read(MAX_FILENAME)
      short = data.decode("ascii").rstrip('\x00')

      files[short] = (pointer,size)

    if args.verbose or args.list:
      print("Filename".ljust(MAX_FILENAME + 1), "Pos".rjust(5), "Size".rjust(5))
      print("".ljust(MAX_FILENAME + 1, "-"), "".rjust(5, "-"), "".rjust(5, "-"))
    for short,data in files.items():
      short_name = zar_to_os(short)
      pointer,size = data
      input.seek(pointer)
      data = input.read(size)
      if args.verbose or args.list:
        print(short_name.ljust(MAX_FILENAME + 1), str(pointer).rjust(5), str(size).rjust(5))
      if not args.list:
        with open(os.path.join(args.output, short_name), "wb") as output:
          output.write(data)



def main():
  args = parser.parse_args()
  print("args", args)

  if not args.extract and not args.list:
    archive(args)
  else:
    extract(args)


if __name__ == "__main__":
  main()
