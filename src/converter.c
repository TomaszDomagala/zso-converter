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




void convert_elf(Elf64_Ehdr ehdr64, Elf64_Shdr *shdrs64, FILE *elf64file) {
    // Elf32_Ehdr ehdr32 = initial_convert_hdr(ehdr64);
    // Elf32_Shdr *shdrs32 = initial_convert_shdrs(shdrs64, ehdr64.e_shnum);

    Elf64_Shdr symtab_shdr = find_shdr64(shdrs64, ehdr64.e_shnum, SHT_SYMTAB);
    Elf64_Sym *symtab64 = load_section64(symtab_shdr, elf64file);

    Elf64_Shdr strtab_shdr = shdrs64[symtab_shdr.sh_link];
    char *strtab = load_section64(strtab_shdr, elf64file);

    Elf64_Shdr shstrtab_shdr = shdrs64[ehdr64.e_shstrndx];
    char *shstrtab = load_section64(shstrtab_shdr, elf64file);

    // Elf64_Shdr text_shdr = find_shdr64_by_name(shdrs64, ehdr64.e_shnum, ".text", shstrtab);


    // print sections offsets
    // for (int i = 0; i < ehdr64.e_shnum; i++) {
    //     Elf64_Shdr shdr = shdrs64[i];
    //     printf("%s: %d\n", shstrtab + shdr.sh_name, shdr.sh_offset);
    // }

    struct section64* sections;
    sections = malloc(sizeof(struct section64) * ehdr64.e_shnum);

    for (int i = 0; i < ehdr64.e_shnum; i++) {
        Elf64_Shdr shdr = shdrs64[i];
        sections[i].header = shdr;
        sections[i].data = load_section64(shdr, elf64file);
    }

    for (int i = 0; i < ehdr64.e_shnum; i++) {
        Elf64_Shdr shdr = shdrs64[i];
        printf("%s: %ld\n", shstrtab + shdr.sh_name, shdr.sh_offset);
    }


    // // find symbols "foo" and "bar"
    // uint64_t symnum = symtab_shdr.sh_size / symtab_shdr.sh_entsize;
    // Elf64_Sym *foo_sym = find_symbol_by_name(symtab64, symnum, "foo", strtab);
    // Elf64_Sym *bar_sym = find_symbol_by_name(symtab64, symnum, "bar", strtab);

    // bar_sym->st_value = foo_sym->st_value;
    // bar_sym->st_size = foo_sym->st_size;
    // bar_sym->st_info = foo_sym->st_info;
    // bar_sym->st_other = foo_sym->st_other;
    // bar_sym->st_shndx = foo_sym->st_shndx;


    FILE *newelf64file = fopen("new.elf", "wb");
    if (newelf64file == NULL) {
        sysfatal("fopen", "cannot create new.elf file");
    }

    if (-1 == fseek(elf64file, sizeof(Elf64_Ehdr), SEEK_SET)) {
        sysfatal("fseek", "cannot seek to the beginning of the file");
    }


    size_t total = 0;
    for (int i = 0; i < ehdr64.e_shnum; i++) {
        struct section64* section = &sections[i];
        if(section->header.sh_addr != 0) {
            fatalf("section %d has non-zero sh_addr", i);
        }

        long pos = ftell(newelf64file);
        section->header.sh_offset = pos;

        if(section->header.sh_size == 0) {
            continue;
        }

        fwrite(section->data, section->header.sh_size, 1, newelf64file);
        total += section->header.sh_size;
        if (ferror(newelf64file)) {
            sysfatal("fwrite", "cannot write section data to new.elf file");
        }
    }
    printf("total: %ld\n", total);
    ehdr64.e_shoff = sizeof(Elf64_Ehdr) + total;
    for (int i = 0; i < ehdr64.e_shnum; i++) {
        struct section64* section = &sections[i];

        fwrite(&section->header, sizeof(Elf64_Shdr), 1, newelf64file);
        if (ferror(newelf64file)) {
            sysfatal("fwrite", "cannot write section header to new.elf file");
        }
    }

    if (-1 == fseek(newelf64file, 0, SEEK_SET)) {
        sysfatal("fseek", "cannot seek to the beginning of the file");
    }
    fwrite(&ehdr64, sizeof(Elf64_Ehdr), 1, newelf64file);
    if (ferror(newelf64file)) {
        sysfatal("fwrite", "cannot write file header");
    }


    // char buf[4096];
    // while (true) {
    //     size_t n = fread(buf, 1, sizeof(buf), elf64file);
    //     if (n == 0) {
    //         if (ferror(elf64file)) {
    //             sysfatal("fread", "cannot read from elf64file");
    //         }
    //         break;
    //     }
    //     size_t w = 0;
    //     while (w < n) {
    //         size_t ww = fwrite(buf + w, 1, n - w, newelf64file);
    //         if (ww == 0 && ferror(newelf64file)) {
    //             sysfatal("fwrite", "cannot write to newelf64file");
    //         }
    //         w += ww;
    //     }
    // }

    // // copy modified symtab to new.elf
    // if (-1 == fseek(newelf64file, symtab_shdr.sh_offset, SEEK_SET)) {
    //     sysfatal("fseek", "cannot seek to symtab_shdr.sh_offset");
    // }
    // fwrite(symtab64, symtab_shdr.sh_size, 1, newelf64file);
    // if (ferror(newelf64file)) {
    //     sysfatal("fwrite", "cannot write to newelf64file");
    // }


    // // .rela.text section
    // Elf64_Shdr rela_text_shdr = find_shdr64_by_name(shdrs64, ehdr64.e_shnum, ".rela.text", shstrtab);
    // Elf64_Rela *rela_text = load_shdr64(rela_text_shdr, elf64file);

    // uint64_t relnum = rela_text_shdr.sh_size / rela_text_shdr.sh_entsize;

    // for (uint64_t i = 0; i < relnum; i++) {
    //     Elf64_Rela rela = rela_text[i];
    //     Elf64_Sym sym = symtab64[ELF64_R_SYM(rela.r_info)];
    //     char *sym_name = &strtab[sym.st_name];
    //     // print values in hex
    //     printf("\nst_name: %s\n", sym_name);
    //     printf("r_offset: %lx\n", rela.r_offset);
    //     printf("r_type: %lx\n", ELF64_R_TYPE(rela.r_info));
        
    //     // if (strcmp(sym_name, "doo") == 0) {
    //         // printf("%s\n", sym_name);
    //     // }
    // }

    // uint64_t symnum = symtab_shdr.sh_size / symtab_shdr.sh_entsize;

    // for (uint64_t i = 0; i < symnum; i++) {
    //     Elf64_Sym sym = symtab64[i];
    //     printf("\n");
    //     printf("st_name: %s\n", strtab + sym.st_name);
    //     printf("st_value: %ld\n", sym.st_value);
    //     printf("st_size: %ld\n", sym.st_size);
    //     printf("st_info: %d\n", sym.st_info);
    //     printf("st_other: %d\n", sym.st_other);
    //     printf("st_shndx: %d\n", sym.st_shndx);
    
    // }




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
