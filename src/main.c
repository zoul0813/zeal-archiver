#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <zos_sys.h>
#include <zos_vfs.h>
#include <zos_video.h>


#include "zar.h"

#define _VERSION "v0.0.0-alpha"
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

void set_color(uint8_t fg) {
    ioctl(DEV_STDOUT, CMD_SET_COLORS, TEXT_COLOR(fg, TEXT_COLOR_BLACK));
}

void print_usage(zos_err_t err)
{
    printf("\nUsage: zar [xlvf] input_file.zar output/path\n");
    printf("  -v    verbose\n");
    printf("  -x    extract\n");
    printf("  -l    list files\n");
    printf("  -f    force, overwrite existing files\n\n");
    printf("\nExample:\n");
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

        if (tokens < 2) {
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
                    default: print_usage(ERR_INVALID_PARAMETER);
                }
            }
            index++;
        }

        if(tokens >= (1 + index)) {
            l = strlen(args[0 + index]);
            memcpy(options.input, args[0 + index], l);
        }


        if(tokens >= (2 + index)) {
            l = strlen(args[1 + index]);
            memcpy(options.output, args[1 + index], l);
        }

        if (options.flags & F_VERBOSE) {
            printf("extract: %s\n", options.flags & F_EXTRACT ? "True" : "False");
            printf("list   : %s\n", options.flags & F_LIST ? "True" : "False");
            printf("verbose: %s\n", options.flags & F_VERBOSE ? "True" : "False");
            printf("force  : %s\n", options.flags & F_FORCE ? "True" : "False");
            printf("input  : %s\n", options.input);
            printf("output : %s\n", options.output);
        }
    } else {
        print_usage(ERR_INVALID_PARAMETER);
    }
}

int main(int argc, char** argv)
{
    zos_err_t err = ERR_SUCCESS;
    set_color(TEXT_COLOR_GREEN);
    printf("Zeal Archiver %s\n\n", _VERSION);
    set_color(TEXT_COLOR_WHITE);

    parse_arguments(argc, argv);
    if((options.flags & F_EXTRACT) && options.output[0] == 0x00) {
        set_color(TEXT_COLOR_RED);
        printf("Output destination is required when extract flag is used.\n");
        set_color(TEXT_COLOR_WHITE);
        print_usage(ERR_INVALID_PARAMETER);
    }

    zar_file_t zar_file;
    err = zar_file_load(options.input, &zar_file);
    if (err != ERR_SUCCESS) {
        printf("\nFailed to open %s, %d [%02x]\n", options.input, err, err);
        exit(err);
    }

    if(options.flags & F_VERBOSE) {
        printf("\n");
        printf("    Header: %03s\n", zar_file.header);
        printf("   Version: %d\n", zar_file.version);
        printf("File Count: %d\n", zar_file.file_count);

    }

    if(options.flags & F_LIST) {
        zar_file_list(&zar_file);
    }

    if (options.flags & F_EXTRACT) {
        printf("Extracting %s to %s\n", options.input, options.output);

        // dirty trick for determining if the dir exists or not
        zos_dev_t output_dir;
        output_dir = opendir(options.output);
        if(output_dir >= 0) close(output_dir);

        // if it exists, or the force flag is not present, error ...
        if((output_dir >= 0) && !(options.flags & F_FORCE)) {
            printf("\nOutput exists, use `f` flag to force: %s, %d [%02x]\n", options.output, err, err);
            exit(err);
        }

        set_color(TEXT_COLOR_LIGHT_GRAY);
        if(output_dir == -ERR_NO_SUCH_ENTRY) {
            printf("Creating %s\n", options.output);
            err = mkdir(options.output);
            if(err != ERR_SUCCESS) {
                printf("Failed to create %s, %d [%02x]\n", options.output, err, err);
                exit(err);
            }
        } else {
            printf("Overwriting %s\n", options.output);
        }

        // change into the output destination folder
        curdir(CWD);
        chdir(options.output);

        uint8_t i;
        for(i = 0; i < zar_file.file_count; i++) {
            zar_file_entry_t *entry = zar_file.entries[i];
            zar_file_get(&zar_file, entry);
            zos_dev_t fd = open(entry->filename, O_WRONLY | O_CREAT);
            if(fd < 0) {
                printf("Failed to open %s%s, %d [%02x]\n", options.output, entry->filename, -fd, -fd);
                continue; // try the next file?
            }
            printf("extracting: %s%s\n", options.output, entry->filename);
            close(fd);
        }
        chdir(CWD);
    }

    printf("Closing %s\n", options.input);
    err = zar_file_close(&zar_file);

    printf("Exitting %d [%02x]\n", err, err);
    return err;
}
