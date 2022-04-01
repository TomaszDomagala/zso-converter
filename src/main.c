#include <elf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fatal.h"
#include "prints.h"
#include "converter.h"

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
        sysfatalf("fopen", "Could not open %s\n", filename);
    }
    fread(&ret.ehdr, sizeof(ret.ehdr), 1, ret.file);
    if (ferror(ret.file)) {
        sysfatalf("fread", "Could not read %s\n", filename);
    }

    if (ret.ehdr.e_type != ET_REL) {
        fatalf("%s is not an ET_REL file\n", filename);
    }
    if (ret.ehdr.e_machine != EM_X86_64) {
        fatalf("%s is not an EM_X86_64 file\n", filename);
    }
    if (ret.ehdr.e_entry != 0) {
        // Assert that entry point is not present.
        warn("Assertion failed: entry point found, but not expected\n");
    }
    if (ret.ehdr.e_phoff != 0) {
        // Assert that program header offset is not present.
        warn("Assertion failed: program header offset found, but not expected\n");
    }
    if (ret.ehdr.e_phentsize != 0) {
        // Assert that program header entry size is not present.
        warn("Assertion failed: program header entry size found, but not expected\n");
    }
    if (ret.ehdr.e_phnum != 0) {
        // Assert that program header number is not present.
        warn("Assertion failed: program header number found, but not expected\n");
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
    
    // if (argc != 4) {
    //     fatalf("Usage: %s <ET_REL file> <functions file> <output ET_REL file>\n", argv[0]);
    // }
    if (argc != 2){ // TODO: remove this and replace with argc == 4 version
        fatalf("Usage: %s <ET_REL file>\n", argv[0]);
    }

    struct Elf64_relfile elf64 = read_relfile(argv[1]);

    convert_elf(elf64.ehdr, elf64.shdrs, elf64.file);

    // print_ehdr(&elf64.ehdr);
    // // print section header table
    // printf("Section header table:\n");
    // for (int i = 0; i < elf64.ehdr.e_shnum; i++) {
    //     print_shdr(elf64.shdrs + i);
    //     printf("\n");
    // }

   

    return 0;
}