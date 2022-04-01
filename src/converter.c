#include "converter.h"

#include <elf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fatal.h"

struct section64 {
    Elf64_Shdr header;
    void *data;
};

struct section32 {
    Elf32_Shdr header;
    void *data;
};

Elf32_Ehdr initial_convert_hdr(Elf64_Ehdr ehdr64);
Elf32_Shdr initial_convert_shdr(Elf64_Shdr shdr64);
Elf64_Shdr find_shdr64(Elf64_Shdr *shdrs64, Elf64_Half shnum, Elf64_Word sh_type);
Elf64_Shdr find_shdr64_by_name(Elf64_Shdr *shdrs64, Elf64_Half shnum, const char *name, char *shstrtab);
void *load_section64(Elf64_Shdr shdr, FILE *elf64file);

Elf64_Sym *find_symbol_by_name(Elf64_Sym *syms64, Elf64_Half num_syms, const char *name, char *strtab);
void write_section32_data(FILE *file, struct section32 *section);

void convert_elf(Elf64_Ehdr ehdr64, Elf64_Shdr *shdrs64, FILE *elf64file) {
    struct section32 *sections;
    sections = malloc(sizeof(struct section32) * ehdr64.e_shnum);

    for (int i = 0; i < ehdr64.e_shnum; i++) {
        Elf64_Shdr shdr = shdrs64[i];
        sections[i].header = initial_convert_shdr(shdr);
        sections[i].data = load_section64(shdr, elf64file);
    }

    Elf32_Ehdr ehdr32 = initial_convert_hdr(ehdr64);

    for (int i = 0; i < ehdr32.e_shnum; i++) {
        Elf32_Shdr ehdr32 = sections[i].header;
        printf("\n");
        printf("sh_name: %d\n", ehdr32.sh_name);
        printf("sh_type: %d\n", ehdr32.sh_type);
        printf("sh_flags: %d\n", ehdr32.sh_flags);
        printf("sh_addr: %d\n", ehdr32.sh_addr);
        printf("sh_offset: %d\n", ehdr32.sh_offset);
        printf("sh_size: %d\n", ehdr32.sh_size);
        printf("sh_link: %d\n", ehdr32.sh_link);
        printf("sh_info: %d\n", ehdr32.sh_info);
        printf("sh_addralign: %d\n", ehdr32.sh_addralign);
        printf("sh_entsize: %d\n", ehdr32.sh_entsize);
    }

    printf("e_shnum: %d\n", ehdr32.e_shnum);
    FILE *newfile = fopen("newelf.o", "wb");
    if (!newfile) {
        sysfatal("fopen", "could not open newelf.o\n");
    }

    if (-1 == fseek(newfile, sizeof(Elf32_Ehdr), SEEK_SET)) {
        sysfatal("fseek", "could not seek to beginning of data\n");
    }

    for (int i = 0; i < ehdr32.e_shnum; i++) {
        write_section32_data(newfile, &sections[i]);
    }
    ehdr32.e_shoff = ftell(newfile);

    for (int i = 0; i < ehdr32.e_shnum; i++) {
        Elf32_Shdr shdr = sections[i].header;
        fwrite(&shdr, sizeof(Elf32_Shdr), 1, newfile);
        if (ferror(newfile)) {
            sysfatal("fwrite", "could not write section header to file\n");
        }
    }
    if (-1 == fseek(newfile, 0, SEEK_SET)) {
        sysfatal("fseek", "could not seek to beginning of file\n");
    }
    printf("e_shoff: %d\n", ehdr32.e_shoff);
    printf("e_ehsize: %d\n", ehdr32.e_ehsize);
    fwrite(&ehdr32, sizeof(Elf32_Ehdr), 1, newfile);
    if (ferror(newfile)) {
        sysfatal("fwrite", "could not write elf header to file\n");
    }
}

void *load_section64(Elf64_Shdr shdr, FILE *elf64file) {
    if (-1 == fseek(elf64file, shdr.sh_offset, SEEK_SET)) {
        sysfatalf("fseek", "could not seek to %ld\n", shdr.sh_offset);
    }
    void *shdr_data = malloc(shdr.sh_size);
    fread(shdr_data, shdr.sh_size, 1, elf64file);
    return shdr_data;
}

/**
 * @brief Finds the section header with the given type.
 * Fails if there is no such section header, or if there are multiple section headers with the given type.
 *
 * @param shdrs64 The section header table.
 * @param shnum The number of section headers in the table.
 * @param sh_type The type of section header to find.
 * @return Elf64_Shdr The section header with the given type.
 */
Elf64_Shdr find_shdr64(Elf64_Shdr *shdrs64, Elf64_Half shnum, Elf64_Word sh_type) {
    Elf64_Half index;
    int found = 0;

    for (Elf64_Half i = 0; i < shnum; i++) {
        if (shdrs64[i].sh_type == sh_type) {
            found++;
            index = i;
        }
    }
    if (found == 0) {
        fatalf("no section header with type %d\n", sh_type);
    } else if (found > 1) {
        fatalf("multiple (%d) section headers with type %d\n", found, sh_type);
    }
    return shdrs64[index];
}

Elf64_Shdr find_shdr64_by_name(Elf64_Shdr *shdrs64, Elf64_Half shnum, const char *name, char *shstrtab) {
    Elf64_Half index;
    int found = 0;

    for (Elf64_Half i = 0; i < shnum; i++) {
        if (strcmp(shstrtab + shdrs64[i].sh_name, name) == 0) {
            found++;
            index = i;
        }
    }

    if (found == 0) {
        fatalf("no section header with name %s\n", name);
    } else if (found > 1) {
        fatalf("multiple (%d) section headers with name %s\n", found, name);
    }
    return shdrs64[index];
}

Elf64_Sym *find_symbol_by_name(Elf64_Sym *syms64, Elf64_Half num_syms, const char *name, char *strtab) {
    Elf64_Half index;
    int found = 0;

    for (Elf64_Half i = 0; i < num_syms; i++) {
        if (strcmp(strtab + syms64[i].st_name, name) == 0) {
            found++;
            index = i;
        }
    }

    if (found == 0) {
        fatalf("no symbol with name %s\n", name);
    } else if (found > 1) {
        fatalf("multiple (%d) symbols with name %s\n", found, name);
    }
    return syms64 + index;
}

Elf32_Ehdr initial_convert_hdr(Elf64_Ehdr ehdr64) {
    Elf32_Ehdr ehdr32;
    memcpy(ehdr32.e_ident, ehdr64.e_ident, sizeof(ehdr32.e_ident));
    ehdr32.e_ident[EI_CLASS] = ELFCLASS32;
    ehdr32.e_type = ET_REL;
    ehdr32.e_machine = EM_386;
    ehdr32.e_version = EV_CURRENT;
    ehdr32.e_entry = 0;
    ehdr32.e_phoff = 0;
    ehdr32.e_shoff = (Elf32_Off)ehdr64.e_shoff;  // TODO: fix this, different int sizes
    ehdr32.e_flags = ehdr64.e_flags;
    ehdr32.e_ehsize = (Elf32_Half)52;
    ehdr32.e_phentsize = 0;
    ehdr32.e_phnum = 0;
    ehdr32.e_shentsize = sizeof(Elf32_Shdr);
    ehdr32.e_shnum = ehdr64.e_shnum;
    ehdr32.e_shstrndx = ehdr64.e_shstrndx;

    return ehdr32;
}

/**
 * @brief Initial convert symbol table from 64-bit to 32-bit.
 *
 * @param shdr64 The elf64 section header table.
 * @return Elf32_Shdr The converted section header table.
 */
Elf32_Shdr initial_convert_shdr(Elf64_Shdr shdr64) {
    Elf32_Shdr shdr32;

    shdr32.sh_name = (Elf32_Word)shdr64.sh_name;
    shdr32.sh_type = (Elf32_Word)shdr64.sh_type;
    shdr32.sh_flags = (Elf32_Word)shdr64.sh_flags;
    shdr32.sh_addr = (Elf32_Addr)shdr64.sh_addr;
    shdr32.sh_offset = (Elf32_Off)shdr64.sh_offset;
    shdr32.sh_size = (Elf32_Word)shdr64.sh_size;
    shdr32.sh_link = (Elf32_Word)shdr64.sh_link;
    shdr32.sh_info = (Elf32_Word)shdr64.sh_info;
    shdr32.sh_addralign = (Elf32_Word)shdr64.sh_addralign;
    shdr32.sh_entsize = (Elf32_Word)shdr64.sh_entsize;

    return shdr32;
}

void write_section32_data(FILE *file, struct section32 *section) {
    if (section->header.sh_type == SHT_NULL) {
        return;
    }
    if (section->header.sh_size == 0) {
        return;
    }
    printf("Writing section\n");

    long current_pos = ftell(file);
    if (section->header.sh_addralign > 1 && current_pos % section->header.sh_addralign != 0) {
        long padding = section->header.sh_addralign - (current_pos % section->header.sh_addralign);
        void *padding_data = calloc(padding, 1);
        fwrite(padding_data, padding, 1, file);
        if (ferror(file)) {
            sysfatal("fwrite", "could not write padding data to file\n");
        }
        free(padding_data);
        current_pos += padding;
    }

    section->header.sh_offset = current_pos;
    fwrite(section->data, section->header.sh_size, 1, file);
    if (ferror(file)) {
        sysfatal("fwrite", "could not write section data to file\n");
    }
}
