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
    if (ret.ehdr.e_type != ET_REL) {
        fatal("%s is not an ET_REL file\n", filename);
    }
    if (ret.ehdr.e_machine != EM_X86_64){
        fatal("%s is not an EM_X86_64 file\n", filename);
    }
    if(ret.ehdr.e_entry != 0) {
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
    if (argc != 4) {
        fatal("Usage: %s <ET_REL file> <functions file> <output ET_REL file>\n", argv[0]);
    }
    struct Elf64_relfile elf64 = read_relfile(argv[1]);

    print_ehdr(&elf64.ehdr);
    // print section header table
    // printf("Section header table:\n");
    // for (int i = 0; i < elf64.ehdr.e_shnum; i++) {
    //     print_shdr(elf64.shdrs + i);
    //     printf("\n");
    // }


    Elf32_Ehdr ehdr32;
    memcpy(&ehdr32.e_ident, &elf64.ehdr.e_ident, sizeof(ehdr32.e_ident));
    ehdr32.e_ident[EI_CLASS] = ELFCLASS32;
    ehdr32.e_type = ET_REL;
    ehdr32.e_machine = EM_386;
    ehdr32.e_version = EV_CURRENT;
    ehdr32.e_entry = 0;
    ehdr32.e_phoff = 0;
    ehdr32.e_shoff = elf64.ehdr.e_shoff; // TODO: fix this, different int sizes
    ehdr32.e_flags = elf64.ehdr.e_flags;
    ehdr32.e_ehsize = sizeof(Elf32_Ehdr);
    ehdr32.e_phentsize = 0;
    ehdr32.e_phnum = 0;
    ehdr32.e_shentsize = elf64.ehdr.e_shentsize;
    ehdr32.e_shnum = elf64.ehdr.e_shnum;
    ehdr32.e_shstrndx = elf64.ehdr.e_shstrndx;

    FILE* file32 = fopen(argv[3], "wb");
    if (!file32) {
        sysfatal("fopen", "Could not open %s\n", argv[3]);
    }

    if(-1 == fseek(elf64.file, 0, SEEK_SET)) {
        sysfatal("fseek", "Could not set position to 0");
    }
    char buf[4096];
    while(!feof(elf64.file)) {
        size_t read = fread(buf, 1, sizeof(buf), elf64.file);
        if (read <= 0 && ferror(elf64.file) != 0) {
            sysfatal("fread", "Could not read %s\n", argv[1]);
        }
        if(read == 0) {
            break;
        }
        if(read > fwrite(buf, 1, read, file32)) {
            sysfatal("fwrite", "Could not write to %s\n", argv[3]);
        }
    }
    clearerr(elf64.file);

    if (-1 == fseek(file32, 0, SEEK_SET)) {
        sysfatal("fseek", "Could not set position to 0");
    }
    if (-1 == fwrite(&ehdr32, sizeof(ehdr32), 1, file32)) {
        sysfatal("fwrite", "Could not write to %s\n", argv[3]);
    }

    // Erase file32 section header table
    memset(buf, 0, sizeof(buf));


    return 0;
}