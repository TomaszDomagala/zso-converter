#include "converter.h"

#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "fatal.h"

Elf32_Ehdr initial_convert_hdr(Elf64_Ehdr ehdr64);
Elf32_Shdr *initial_convert_shdrs(Elf64_Shdr *shdrs64, int shnum);
Elf64_Shdr find_shdr64(Elf64_Shdr *shdrs64, Elf64_Half shnum, Elf64_Word sh_type);
void *load_shdr64(Elf64_Shdr shdr, FILE *elf64file);


void convert_elf(Elf64_Ehdr ehdr64, Elf64_Shdr *shdrs64, FILE *elf64file) {
    Elf32_Ehdr ehdr32 = initial_convert_hdr(ehdr64);
    Elf32_Shdr *shdrs32 = initial_convert_shdrs(shdrs64, ehdr64.e_shnum);

    Elf64_Shdr symtab = find_shdr64(shdrs64, ehdr64.e_shnum, SHT_SYMTAB);
    printf("symtab: %ld\n", symtab.sh_offset);
}


void *load_shdr64(Elf64_Shdr shdr, FILE *elf64file){
    if (-1 == fseek(elf64file, shdr.sh_offset, SEEK_SET)) {
        sysfatalf("fseek", "Could not seek to %ld\n", shdr.sh_offset);
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
Elf64_Shdr find_shdr64(Elf64_Shdr *shdrs64, Elf64_Half shnum, Elf64_Word sh_type){
    Elf64_Half index;
    bool found = false;
    
    for (Elf64_Half i = 0; i < shnum; i++) {
        if (shdrs64[i].sh_type == sh_type) {
            if(found){
                fatalf("Found multiple sections of type %d\n", sh_type);
            }
            index = i;
            found = true;
        }
    }
    if(!found){
        fatalf("Could not find section of type %d\n", sh_type);
    }
    return shdrs64[index];
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

Elf32_Shdr *initial_convert_shdrs(Elf64_Shdr *shdrs64, int shnum) {
    Elf32_Shdr *shdrs32 = malloc(sizeof(Elf32_Shdr) * shnum);
    if (shdrs32 == NULL) {
        sysfatal("malloc", "Could not allocate memory for section header table");
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
