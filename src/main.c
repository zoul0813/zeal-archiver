#include <stdint.h>
#include <stddef.h>
#include <zos_sys.h>
#include <zos_vfs.h>
#include <zos_video.h>


#include "zar.h"
#include "stdutils.h"

#define _VERSION "v0.0.0-beta"
#define NIL      0x00

#define F_NONE    0x00
#define F_EXTRACT 0x01
#define F_LIST    0x02
#define F_VERBOSE 0x04
#define F_FORCE   0x08

typedef struct {
        uint8_t flags;
        char input[PATH_MAX];
        char output[PATH_MAX];
} options_t;

options_t options;
char CWD[PATH_MAX];
uint8_t buffer[1024];

void set_color(uint8_t fg)
{
    ioctl(DEV_STDOUT, CMD_SET_COLORS, TEXT_COLOR(fg, TEXT_COLOR_BLACK));
}

void print_usage(zos_err_t err)
{
    if (err != ERR_SUCCESS) {
        set_color(TEXT_COLOR_RED);
        printf("Invalid Parameter\n\n");
        set_color(TEXT_COLOR_WHITE);
    }

    printf("\nUsage: zar [xlvf] input_file.zar output/path\n");
    printf("  -x    extract\n");
    printf("  -l    list files\n");
    printf("  -v    verbose\n");
    printf("  -f    force, overwrite existing files\n");
    printf("  -h    this help message\n");
    printf("\n\nExample:\n");
    printf("\n  zar xl input.zar B:/output/path/\n\n");

    if (err != ERR_SUCCESS) {
        exit(err);
    }
}

void parse_arguments(int argc, char** argv)
{
    // clear options
    options.flags = F_NONE;
    memset(&options.input, 0, PATH_MAX);
    memset(&options.output, 0, PATH_MAX);

    uint8_t i, l, index = 0;
    if (argc == 1) {
        const char* args[3] = {NIL, NIL, NIL};
        const char* token   = strtok(argv[0], " ");
        uint8_t tokens      = 0;
        while (token != NIL) {
            args[tokens] = token;
            token        = strtok(NIL, " ");
            ++tokens;
        }

        if (tokens < 1) {
            print_usage(ERR_INVALID_PARAMETER);
        }

        if (tokens >= 1) {
            // process the first argument as flags
            l = strlen(args[0]);
            for (i = 0; i < l; i++) {
                char c = argv[0][i];
                switch (c) {
                    case 'x': options.flags |= F_EXTRACT; break;
                    case 'l': options.flags |= F_LIST; break;
                    case 'v': options.flags |= F_VERBOSE; break;
                    case 'f': options.flags |= F_FORCE; break;
                    case 'h': print_usage(ERR_SUCCESS); exit(ERR_SUCCESS);
                    default: print_usage(ERR_INVALID_PARAMETER);
                }
            }
            index++;
        }

        if (tokens >= (1 + index)) {
            l = strlen(args[0 + index]);
            memcpy(options.input, args[0 + index], l);
        }


        if (tokens >= (2 + index)) {
            l = strlen(args[1 + index]);
            memcpy(options.output, args[1 + index], l);
            if(options.output[l-1] != '/') {
                options.output[l] = '/';
            }
        }
    } else {
        print_usage(ERR_INVALID_PARAMETER);
    }
}

zos_err_t list_files(zar_file_t* zar_file)
{
    zos_err_t err = ERR_SUCCESS;

    set_color(TEXT_COLOR_WHITE);
    printf("Filename        Size   Pos\n");
    printf("------------  ------ -----\n");

    uint8_t i;
    for (i = 0; i < zar_file->file_count; i++) {
        zar_file_entry_t* entry = zar_file->entries[i];
        if (entry != NULL) {
            printf("%-12s  %5dB %5d\n", entry->filename, entry->size, entry->position);
        }
    }

    return err;
}

int main(int argc, char** argv)
{
    zos_err_t err = ERR_SUCCESS;
    parse_arguments(argc, argv);

    if (options.flags & F_VERBOSE) {
        set_color(TEXT_COLOR_GREEN);
        fprintf(DEV_STDOUT, "Zeal Archiver %s\n\n", _VERSION);
        set_color(TEXT_COLOR_WHITE);
    }

    if (options.flags & F_VERBOSE) {
        printf("Arguments:\n");
        set_color(TEXT_COLOR_LIGHT_GRAY);
        printf("   extract: %s\n", options.flags & F_EXTRACT ? "True" : "False");
        printf("      list: %s\n", options.flags & F_LIST ? "True" : "False");
        printf("   verbose: %s\n", options.flags & F_VERBOSE ? "True" : "False");
        printf("     force: %s\n", options.flags & F_FORCE ? "True" : "False");
        if (options.input[0] != 0x00) {
            printf("     input: ");
            set_color(TEXT_COLOR_YELLOW);
            printf("%s\n", options.input);
            set_color(TEXT_COLOR_WHITE);
        }
        if (options.output[0] != 0x00) {
            printf("    output: ");
            set_color(TEXT_COLOR_YELLOW);
            printf("%s\n", options.output);
            set_color(TEXT_COLOR_WHITE);
        }
        set_color(TEXT_COLOR_WHITE);
    }

    if ((options.flags & F_EXTRACT) && options.output[0] == 0x00) {
        set_color(TEXT_COLOR_RED);
        printf("Output destination is required when extract flag is used.\n");
        set_color(TEXT_COLOR_WHITE);
        print_usage(ERR_INVALID_PARAMETER);
    }

    zar_file_t zar_file;
    err = zar_file_open(options.input, &zar_file);
    if (err != ERR_SUCCESS) {
        printf("\nFailed to open %s, %d [%02x]\n", options.input, err, err);
        exit(err);
    }

    if (options.flags & F_VERBOSE) {
        printf("\n");
        printf("Header:\n");
        set_color(TEXT_COLOR_LIGHT_GRAY);
        printf("    Header: %03s\n", zar_file.header);
        printf("   Version: %d\n", zar_file.version);
        printf("File Count: %d\n\n", zar_file.file_count);
        set_color(TEXT_COLOR_WHITE);
    }

    if (options.flags & F_LIST) {
        list_files(&zar_file);
    }

    if (options.flags & F_EXTRACT) {
        if (options.flags & F_VERBOSE) {
            printf("Extracting %s to %s\n", options.input, options.output);
        }

        // dirty trick for determining if the dir exists or not
        zos_dev_t output_dir;
        output_dir = opendir(options.output);
        if (output_dir >= 0)
            close(output_dir);

        // if it exists, or the force flag is not present, error ...
        if ((output_dir >= 0) && !(options.flags & F_FORCE)) {
            printf("\nOutput exists, use `f` flag to force: %s, %d [%02x]\n", options.output, err, err);
            exit(err);
        }

        set_color(TEXT_COLOR_LIGHT_GRAY);
        if (output_dir == -ERR_NO_SUCH_ENTRY) {
            if (options.flags & F_VERBOSE) {
                printf("Creating %s\n", options.output);
            }
            err = mkdir(options.output);
            if (err != ERR_SUCCESS) {
                printf("Failed to create %s, %d [%02x]\n", options.output, err, err);
                exit(err);
            }
        } else {
            if (options.flags & F_VERBOSE) {
                printf("Overwriting %s\n", options.output);
            }
        }

        // change into the output destination folder
        curdir(CWD);
        chdir(options.output);

        uint8_t i;
        for (i = 0; i < zar_file.file_count; i++) {
            zar_file_entry_t* entry = zar_file.entries[i];

            // open output file for writing
            zos_dev_t fd = open(entry->filename, O_WRONLY | O_CREAT);
            if (fd < 0) {
                printf("Failed to open %s%s, %d [%02x]\n", options.output, entry->filename, -fd, -fd);
                continue; // try the next file?
            }
            if (options.flags & F_VERBOSE) {
                printf("extracting: %s%s\n", options.output, entry->filename);
            }

            // read 1k at a time
            uint16_t size = sizeof(buffer);
            do {
                err = zar_file_read(&zar_file, entry, buffer, &size);
                if (size > 0) {
                    if (err != ERR_SUCCESS) {
                        printf("Failed to read %d bytes from %s for %s\n", size, options.input, entry->filename);
                        close(fd);
                        goto file_loop_done;
                    }
                    err = write(fd, &buffer, &size);
                    if (err != ERR_SUCCESS) {
                        printf("Failed to write %d bytes to %s%s\n", size, options.output, entry->filename);
                        close(fd);
                        goto file_loop_done;
                    }
                }
            } while (size > 0);
            close(fd);
        }
file_loop_done:
        chdir(CWD);
    }

    err = zar_file_close(&zar_file);
    return err;
}
