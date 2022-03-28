#include <elf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "prints.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fatal("Usage: %s <ET_REL file> <functions file> <output ET_REL file>\n", argv[0]);
    }
    FILE *in = fopen(argv[1], "rb");

    if (!in) {
        sysfatal("fopen", "Could not open %s\n", argv[1]);
    }

    Elf64_Ehdr ehdr;
    if (-1 == fread(&ehdr, sizeof(ehdr), 1, in)) {
        sysfatal("fread", "Could not read %s\n", argv[1]);
    }
    if (ehdr.e_shoff == 0) {
        fatal("No section header table found\n");
    }
    print_ehdr(&ehdr);
    // TODO: handle e_shnum and e_shstrndx in initial entry

    size_t shdrs_size = ehdr.e_shentsize * ehdr.e_shnum;
    Elf64_Shdr *shdrs = malloc(shdrs_size);

    if (shdrs == NULL) {
        sysfatal("malloc", "Could not allocate memory for section header table");
    }
    if (-1 == fseek(in, ehdr.e_shoff, SEEK_SET)) {
        sysfatal("fseek", "Could not set position to section header table");
    }

    size_t sections_read = 0;
    while (sections_read < ehdr.e_shnum) {
        size_t read = fread(shdrs + sections_read, ehdr.e_shentsize, ehdr.e_shnum - sections_read, in);
        if (read <= 0 && ferror(in) != 0) {
            sysfatal("fread", "Could not read section header table");
        }
        sections_read += read;
    }

    Elf64_Shdr shstrtab_section = shdrs[ehdr.e_shstrndx];
    char *shstrtab = malloc(shstrtab_section.sh_size);
    if (shstrtab == NULL) {
        sysfatal("malloc", "Could not allocate memory for string table");
    }
    if (-1 == fseek(in, shstrtab_section.sh_offset, SEEK_SET)) {
        sysfatal("fseek", "Could not set position to string table");
    }
    if (-1 == fread(shstrtab, shstrtab_section.sh_size, 1, in)) {
        sysfatal("fread", "Could not read string table");
    }

    for (int i = 0; i < ehdr.e_shnum; i++) {
        printf("\nSection %d: %s\n", i, shstrtab + shdrs[i].sh_name);
        print_shdr(shdrs + i);
    }

    Elf64_Shdr symtab_section;

    size_t symtab_found = 0;
    for (int i = 0; i < ehdr.e_shnum; i++) {
        if (shdrs[i].sh_type == SHT_SYMTAB) {
            if (symtab_found) {
                fatal("Multiple symbol tables found\n");
            }

            symtab_section = shdrs[i];
            symtab_found += 1;
        }
    }
    if (!symtab_found) {
        fatal("No symbol table found\n");
    }

    Elf64_Sym *symtab = malloc(symtab_section.sh_size);
    if (symtab == NULL) {
        sysfatal("malloc", "Could not allocate memory for symbol table");
    }
    if (-1 == fseek(in, symtab_section.sh_offset, SEEK_SET)) {
        sysfatal("fseek", "Could not set position to symbol table");
    }
    if (-1 == fread(symtab, symtab_section.sh_size, 1, in)) {
        sysfatal("fread", "Could not read symbol table");
    }

    Elf64_Shdr strtab_section = shdrs[symtab_section.sh_link];
    char *strtab = malloc(strtab_section.sh_size);
    if (strtab == NULL) {
        sysfatal("malloc", "Could not allocate memory for string table");
    }
    if (-1 == fseek(in, strtab_section.sh_offset, SEEK_SET)) {
        sysfatal("fseek", "Could not set position to string table");
    }
    if (-1 == fread(strtab, strtab_section.sh_size, 1, in)) {
        sysfatal("fread", "Could not read string table");
    }

    for (int i = 0; i < symtab_section.sh_size / symtab_section.sh_entsize; i++) {
        printf("\nSymbol %d: %s\n", i, strtab + symtab[i].st_name);
        print_sym(symtab + i);
        printf("symbol type: %d\n", ELF64_ST_TYPE(symtab[i].st_info));
    }

    return 0;
}