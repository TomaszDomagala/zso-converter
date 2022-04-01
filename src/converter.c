#include "converter.h"

#include <elf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fatal.h"

Elf32_Ehdr initial_convert_hdr(Elf64_Ehdr ehdr64);
Elf32_Shdr *initial_convert_shdrs(Elf64_Shdr *shdrs64, int shnum);
Elf64_Shdr find_shdr64(Elf64_Shdr *shdrs64, Elf64_Half shnum, Elf64_Word sh_type);
Elf64_Shdr find_shdr64_by_name(Elf64_Shdr *shdrs64, Elf64_Half shnum, const char *name, char *shstrtab);
void *load_section64(Elf64_Shdr shdr, FILE *elf64file);

Elf64_Sym* find_symbol_by_name(Elf64_Sym *syms64, Elf64_Half num_syms, const char *name, char *strtab);


struct section64 {
    Elf64_Shdr header;
    void *data;
};

void write_section64_data(FILE *file, struct section64 *section){
    if (section->header.sh_type == SHT_NULL) {
        return;
    }
    if (section->header.sh_size == 0) {
        return;
    }

    long current_pos = ftell(file);
    if (section->header.sh_addralign > 1 && current_pos % section->header.sh_addralign != 0){
        long padding = section->header.sh_addralign - (current_pos % section->header.sh_addralign);
        void* padding_data = calloc(padding, 1);
        fwrite(padding_data, padding, 1, file);
        if (ferror(file)){
            sysfatal("fwrite", "could not write padding data to file\n");
        }
        free(padding_data);
        current_pos += padding;
    }
    
    section->header.sh_offset = current_pos;
    fwrite(section->data, section->header.sh_size, 1, file);
    if (ferror(file)){
        sysfatal("fwrite", "could not write section data to file\n");
    }
}


void convert_elf(Elf64_Ehdr ehdr64, Elf64_Shdr *shdrs64, FILE *elf64file) {

    struct section64* sections;
    sections = malloc(sizeof(struct section64) * ehdr64.e_shnum);

    for (int i = 0; i < ehdr64.e_shnum; i++) {
        Elf64_Shdr shdr = shdrs64[i];
        sections[i].header = shdr;
        sections[i].data = load_section64(shdr, elf64file);
    }

    FILE* newfile = fopen("newelf.o", "wb");
    if (!newfile) {
        sysfatal("fopen", "could not open newelf.o\n");
    }

    if (-1 == fseek(newfile, sizeof(Elf64_Shdr), SEEK_SET)) {
        sysfatal("fseek", "could not seek to beginning of data\n");
    }

    for (int i = 0; i < ehdr64.e_shnum; i++) {
        write_section64_data(newfile, &sections[i]);
    }

    ehdr64.e_shoff = ftell(newfile);
    for (int i = 0; i < ehdr64.e_shnum; i++) {
        Elf64_Shdr shdr = sections[i].header;
        fwrite(&shdr, sizeof(Elf64_Shdr), 1, newfile);
        if (ferror(newfile)){
            sysfatal("fwrite", "could not write section header to file\n");
        }
    }
    if (-1 == fseek(newfile, 0, SEEK_SET)) {
        sysfatal("fseek", "could not seek to beginning of file\n");
    }
    fwrite(&ehdr64, sizeof(Elf64_Ehdr), 1, newfile);
    if (ferror(newfile)){
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

Elf64_Sym* find_symbol_by_name(Elf64_Sym *syms64, Elf64_Half num_syms, const char *name, char *strtab){
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
    memcpy(&ehdr32.e_ident, ehdr64.e_ident, sizeof(ehdr32.e_ident));
    ehdr32.e_ident[EI_CLASS] = ELFCLASS32;
    ehdr32.e_type = ET_REL;
    ehdr32.e_machine = EM_386;
    ehdr32.e_version = EV_CURRENT;
    ehdr32.e_entry = 0;
    ehdr32.e_phoff = 0;
    ehdr32.e_shoff = ehdr64.e_shoff;  // TODO: fix this, different int sizes
    ehdr32.e_flags = ehdr64.e_flags;
    ehdr32.e_ehsize = sizeof(Elf32_Ehdr);
    ehdr32.e_phentsize = 0;
    ehdr32.e_phnum = 0;
    ehdr32.e_shentsize = sizeof(Elf32_Shdr);
    ehdr32.e_shnum = ehdr64.e_shnum;
    ehdr32.e_shstrndx = ehdr64.e_shstrndx;

    return ehdr32;
}

/**
 * @brief Initial convert section header table from 64-bit to 32-bit.
 * 
 * @param shdrs64 The elf64 section header table.
 * @param shnum The number of section headers in the table.
 * @return Elf32_Shdr* The converted section header table.
 */
Elf32_Shdr *initial_convert_shdrs(Elf64_Shdr *shdrs64, int shnum) {
    Elf32_Shdr *shdrs32 = malloc(sizeof(Elf32_Shdr) * shnum);
    if (shdrs32 == NULL) {
        sysfatal("malloc", "could not allocate memory for section header table");
    }

    for (int i = 0; i < shnum; i++) {
        shdrs32[i].sh_name = shdrs64[i].sh_name;
        shdrs32[i].sh_type = shdrs64[i].sh_type;
        shdrs32[i].sh_flags = (Elf32_Word)shdrs64[i].sh_flags;
        shdrs32[i].sh_addr = (Elf32_Addr)shdrs64[i].sh_addr;
        shdrs32[i].sh_offset = (Elf32_Off)shdrs64[i].sh_offset;
        shdrs32[i].sh_size = (Elf32_Word)shdrs64[i].sh_size;
        shdrs32[i].sh_link = shdrs64[i].sh_link;
        shdrs32[i].sh_info = shdrs64[i].sh_info;
        shdrs32[i].sh_addralign = (Elf32_Word)shdrs64[i].sh_addralign;
        shdrs32[i].sh_entsize = (Elf32_Word)shdrs64[i].sh_entsize;
    }

    return shdrs32;
}
