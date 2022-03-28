#include <elf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fatal.h"
#include "prints.h"

struct Elf64_relfile {
    Elf64_Ehdr ehdr;
    Elf64_Shdr *shdrs;
    FILE *file;
};

struct Elf64_relfile read_relfile(char *filename) {
    struct Elf64_relfile ret;
    ret.file = fopen(filename, "rb");

    // Load file header
    if (!ret.file) {
        sysfatal("fopen", "Could not open %s\n", filename);
    }
    if (-1 == fread(&ret.ehdr, sizeof(ret.ehdr), 1, ret.file)) {
        sysfatal("fread", "Could not read %s\n", filename);
    }
    if (ret.ehdr.e_phnum != 0) {
        // Assert that program header table is not present.
        warn("Assertion failed: program header table found, but not expected\n");
    }
    if (ret.ehdr.e_shoff == 0) {
        fatal("No section header table found\n");
    }

    // Load section header table
    size_t shdrs_size = ret.ehdr.e_shentsize * ret.ehdr.e_shnum;
    ret.shdrs = malloc(shdrs_size);
    if (ret.shdrs == NULL) {
        sysfatal("malloc", "Could not allocate memory for section header table");
    }
    if (-1 == fseek(ret.file, ret.ehdr.e_shoff, SEEK_SET)) {
        sysfatal("fseek", "Could not set position to section header table");
    }

    size_t sections_read = 0;
    while (sections_read < ret.ehdr.e_shnum) {
        size_t read = fread(ret.shdrs + sections_read, ret.ehdr.e_shentsize, ret.ehdr.e_shnum - sections_read, ret.file);
        if (read <= 0 && ferror(ret.file) != 0) {
            sysfatal("fread", "Could not read section header table");
        }
        sections_read += read;
    }

    return ret;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fatal("Usage: %s <ET_REL file> <functions file> <output ET_REL file>\n", argv[0]);
    }
    struct Elf64_relfile elf64 = read_relfile(argv[1]);

    // print section header table
    printf("Section header table:\n");
    for (int i = 0; i < elf64.ehdr.e_shnum; i++) {
        print_shdr(elf64.shdrs + i);
        printf("\n");
    }

    return 0;
}