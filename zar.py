#!/usr/bin/env python3

import argparse
import os
import re
import struct
from pathlib import Path

parser = argparse.ArgumentParser("zar")
parser.add_argument("-i", "--input", help="Input Folder", required=True)
parser.add_argument("-o", "--output", help="Output File", required=False)
parser.add_argument("-c", "--header", help="Produce a C Header with Indexes", nargs='?', default=False, required=False)
parser.add_argument("-v", "--verbose", help="Verbose output", action="store_true")
parser.add_argument("-x", "--extract", help="Extract Input to Output", action="store_true")
parser.add_argument("-l", "--list", help="List archive files", action="store_true")

MAX_ENTRIES = 255
MAX_BASENAME = 8
MAX_EXTENSION = 3
MAX_FILENAME = MAX_BASENAME + MAX_EXTENSION
FILE_HEADER_SIZE = 4 + MAX_FILENAME  # uint16_t, uint16_t, char[MAX_FILE_NAME]


def create_dir(file_path):
    if "." in os.path.basename(file_path):
        directory = os.path.dirname(file_path)
    else:
        directory = file_path
    if directory and not os.path.exists(directory):
        os.makedirs(directory, exist_ok=True)


def process_paths(input_path, output_path):
    # Extract input directory and filename
    input_dir = os.path.dirname(input_path)
    input_filename = os.path.basename(input_path)
    if not output_path:
        output_path = input_path

    # Check if output path has an extension (meaning it's a file) or is a directory
    output_has_extension = "." in os.path.basename(output_path)

    if output_has_extension:
        output_dir = os.path.dirname(output_path)
        output_filename = os.path.basename(output_path)
    else:
        output_dir = output_path  # Assume it's a directory
        output_filename = os.path.splitext(input_filename)[0] + ".zts"

    # Construct full output path
    full_output_path = os.path.join(output_dir, output_filename)

    return output_dir, output_filename, full_output_path


def zar_to_os(filename):
    return (
        filename[:MAX_BASENAME].rstrip("\x00")
        + "."
        + filename[MAX_BASENAME:].rstrip("\x00")
    )


def generate_zar_filenames(filenames):
    seen = {}
    normalized = {}

    for name in filenames:
        # Remove non-alphanumeric characters (optional)
        base, ext = os.path.splitext(os.path.basename(name))
        base = re.sub(r"[^a-zA-Z0-9]", "", base).ljust(MAX_BASENAME, "\x00")
        ext = re.sub(r"[^a-zA-Z0-9]", "", ext)

        base_name = base[:MAX_BASENAME] + ext[:MAX_EXTENSION]
        count = seen.get(base_name, 0)
        if count > 0:
            short_name = (
                short_name[: MAX_BASENAME - 3]
                + str(count).zfill(3)
                + ext[:MAX_EXTENSION]
            )
        else:
            short_name = base_name
        seen[base_name] = count + 1

        short_name = short_name.ljust(MAX_FILENAME, "\x00")
        normalized[name] = short_name
    return normalized


def archive(args):
    if not args.output:
        print("Output filename required")
        return

    outputDir, outputFilename, outputPath = process_paths(args.input, args.output)
    create_dir(outputPath)

    print("Archiving ", args.input, "to", args.output)
    # get absolute paths
    rel_names = sorted(os.listdir(args.input))
    # filter out hidden dot files
    filenames = [
        os.path.abspath(os.path.join(args.input, file))
        for file in rel_names
        if not os.path.basename(file).startswith(".")
    ]

    file_count = len(filenames)
    if file_count > MAX_ENTRIES:
        print("ZAR has a", MAX_ENTRIES, "file limit")
        return

    header = "ZAR"
    version = 0

    total_size = 0
    position = 0

    with open(args.output, "wb") as output:
        total_size += output.write(header.encode("ascii"))
        total_size += output.write(struct.pack("B", version))
        total_size += output.write(struct.pack("B", file_count))

        zar_files = generate_zar_filenames(filenames)

        position = total_size + (FILE_HEADER_SIZE * len(zar_files))
        for src, short_name in zar_files.items():
            size = os.path.getsize(src)
            short = short_name.encode("ascii")

            # start position of file
            total_size += output.write(struct.pack("<H", position))
            # size of the file
            total_size += output.write(struct.pack("<H", size))
            total_size += output.write(short)

            if args.verbose:
                print(position, size, short)

            # add the filesize to pointer
            position += size

        for src, short_name in zar_files.items():
            with open(src, "rb") as input:
                total_size += output.write(input.read())

    return args.output


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

        for i in range(file_count):
            data = input.read(2)
            pointer = struct.unpack("<H", data)[0]
            data = input.read(2)
            size = struct.unpack("<H", data)[0]
            data = input.read(MAX_FILENAME)
            short = data.decode("ascii").rstrip("\x00")

            files[short] = (pointer, size)

        if args.verbose or args.list:
            print("Index".ljust(5), "Filename".ljust(MAX_FILENAME + 1), "Size".rjust(6), "Pos".rjust(5))
            print("".ljust(5, "-"), "".ljust(MAX_FILENAME + 1, "-"), "".rjust(6, "-"), "".rjust(5, "-"))
        for index, (short, data) in enumerate(files.items()):
            short_name = zar_to_os(short)
            pointer, size = data
            input.seek(pointer)
            data = input.read(size)
            if args.verbose or args.list:
                print(
                    str(index).rjust(5),
                    short_name.ljust(MAX_FILENAME + 1),
                    str(size).rjust(5) + "B",
                    str(pointer).rjust(5),
                )
            if not args.list:
                with open(os.path.join(args.output, short_name), "wb") as output:
                    output.write(data)


def create_c_header(args, inputFile):
    headerFile = args.header
    if not headerFile:
        headerFile = 'zar.h'

    print("Generating C Header", headerFile)
    with open(headerFile, "w") as output:
        with open(inputFile, "rb") as input:
            input.seek(0, os.SEEK_END)
            size = input.tell()
            input.seek(0, os.SEEK_SET)
            header = input.read(3).decode("ascii")
            version = ord(input.read(1))
            file_count = ord(input.read(1))

            output.write("/**\n")
            output.write(" * ZAR File Header for enigma.zar\n")
            output.write(f" * {header}{version}: {file_count} files, {size} bytes\n")
            output.write(" */\n\n")

            for i in range(file_count):
                data = input.read(2)
                pointer = struct.unpack("<H", data)[0]
                data = input.read(2)
                size = struct.unpack("<H", data)[0]
                data = input.read(MAX_FILENAME)
                short = data.decode("ascii").rstrip("\x00")
                short = zar_to_os(short)

                # Remove extension and get the base name
                macro = re.sub(r'[^a-zA-Z0-9]', '_', short)  # Replace non-alphanumeric characters with '_'
                macro = macro.upper()  # Convert to uppercase
                macro = f"ZAR_{macro}"

                output.write(f"#define {macro.ljust(24)} {i}\t\t// at {pointer}, {size} bytes\n")


def main():
    args = parser.parse_args()
    print("args", args)

    inputFile = args.input

    if not args.extract and not args.list:
        inputFile = archive(args)
    else:
        extract(args)

    if not args.header == False:
        create_c_header(args, inputFile)



if __name__ == "__main__":
    main()
