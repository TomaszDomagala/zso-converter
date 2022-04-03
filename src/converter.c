#include "converter.h"

#include <elf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fatal.h"
#include "list.h"

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
// Elf64_Shdr find_shdr64(Elf64_Shdr *shdrs64, Elf64_Half shnum, Elf64_Word sh_type);
// Elf64_Shdr find_shdr64_by_name(Elf64_Shdr *shdrs64, Elf64_Half shnum, const char *name, char *shstrtab);

struct section32 *find_section32(const char *name, Elf32_Ehdr ehdr32, struct section32 *sections32);

void *load_section64(Elf64_Shdr shdr, FILE *elf64file);

Elf64_Sym *find_symbol_by_name(Elf64_Sym *syms64, Elf64_Half num_syms, const char *name, char *strtab);
void write_section32_data(FILE *file, struct section32 *section);

void convert_section32(struct section32 *section, Elf32_Ehdr ehdr32, struct section32 *all_sections);

void create_stubs(Elf32_Ehdr ehdr32, struct section32 *sections);

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
        convert_section32(&sections[i], ehdr32, sections);
    }

    create_stubs(ehdr32, sections);

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
    if (ferror(elf64file)) {
        sysfatalf("fread", "could not read %ld bytes from file\n", shdr.sh_size);
    }

    return shdr_data;
}

/*
Disassembly of section .text:

00000000 <fun_stub>:
   0:	55                   	push   %ebp
   1:	89 e5                	mov    %esp,%ebp
   3:	53                   	push   %ebx
   4:	57                   	push   %edi
   5:	56                   	push   %esi
   6:	83 ec 04             	sub    $0x4,%esp
   9:	83 ec 08             	sub    $0x8,%esp
   c:	8d 1d 1e 00 00 00    	lea    0x1e,%ebx
  12:	c7 44 24 04 23 00 00 	movl   $0x23,0x4(%esp)
  19:	00
  1a:	89 1c 24             	mov    %ebx,(%esp)
  1d:	cb                   	lret

0000001e <fun_stub_out>:
  1e:	8b 45 08             	mov    0x8(%ebp),%eax
  21:	83 c4 04             	add    $0x4,%esp
  24:	5e                   	pop    %esi
  25:	5f                   	pop    %edi
  26:	5b                   	pop    %ebx
  27:	5d                   	pop    %ebp
  28:	c3                   	ret
*/
const unsigned char fun_stub[] = {
    0x55,                       // push %ebp
    0x89, 0xe5,                 // mov %esp,%ebp
    0x53,                       // push %ebx
    0x57,                       // push %edi
    0x56,                       // push %esi
    0x83, 0xec, 0x04,           // sub $0x4,%esp
    0x83, 0xec, 0x08,           // sub $0x8,%esp
    0x8d, 0x1d, 0x1e, 0x00, 0x00, 0x00, // lea 0x1e,%ebx
    0xc7, 0x44, 0x24, 0x04, 0x23, 0x00, 0x00, 0x00, // movl $0x23,0x4(%esp)
    0x00,
    0x89, 0x1c, 0x24,           // mov %ebx,(%esp)
    0xcb,                       // lret
};
void create_stubs(Elf32_Ehdr ehdr32, struct section32 *sections) {
    struct section32 *symtab = find_section32(".symtab", ehdr32, sections);
    struct section32 *strtab = find_section32(".strtab", ehdr32, sections);
    char* strings = strtab->data;

    Elf32_Sym *syms = symtab->data;
    // list of global functions
    list_t *funcs = list_create(sizeof(Elf32_Sym *));

    Elf32_Word symnum = symtab->header.sh_size / symtab->header.sh_entsize;
    for (Elf32_Word i = 0; i < symnum; i++) {
        Elf32_Sym *sym = &syms[i];

        if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC && ELF32_ST_BIND(sym->st_info) == STB_GLOBAL) {
            list_add(funcs, sym);
        }
    }

    iterate_list(funcs, node){
        Elf32_Sym *sym = list_element(node);
        printf("function name: %s\n", strings + sym->st_name);
    }

    list_free(funcs);
}

/**
 * @brief check if a section is a symbol table
 *
 * @param shdr section header
 * @return true if section is a symbol table
 * @return false if section is not a symbol table
 */
bool is_section_symtab(Elf32_Shdr shdr) {
    return shdr.sh_type == SHT_SYMTAB;
}

/**
 * @brief Converts Elf64_Sym to Elf32_Sym
 * This function expects section to be initiali converted section with untouched data
 * containing Elf64_Sym table.
 * @param section 32-bit symtab section with not yet converted symbols data
 */
void convert_symtab_section(struct section32 *section) {
    Elf32_Word snum = section->header.sh_size / sizeof(Elf64_Sym);
    Elf32_Sym *syms32 = malloc(sizeof(Elf32_Sym) * snum);

    for (Elf32_Word i = 0; i < snum; i++) {
        Elf64_Sym *sym64 = (Elf64_Sym *)(section->data) + i;
        Elf32_Sym *sym32 = syms32 + i;
        sym32->st_name = sym64->st_name;
        sym32->st_value = sym64->st_value;
        sym32->st_size = sym64->st_size;
        // could use ELF32_ST_INFO and friends, but st_info for both 32 and 64 bit is the same.
        sym32->st_info = sym64->st_info;
        sym32->st_other = sym64->st_other;
        sym32->st_shndx = sym64->st_shndx;
    }
    free(section->data);
    section->data = syms32;
    section->header.sh_entsize = sizeof(Elf32_Sym);
    section->header.sh_size = sizeof(Elf32_Sym) * snum;
}

/**
 * @brief Check if a section is a RELA table
 *
 * @param shdr section header
 * @return true if section is a RELA table
 * @return false if section is not a RELA table
 */
bool is_rela_section(Elf32_Shdr shdr) {
    return shdr.sh_type == SHT_RELA;
}

/**
 * @brief Converts Elf64_Rela to Elf32_Rela and supported relocation types.
 * This function expects section to be initiali converted section with untouched data
 *
 * @param section 32-bit rela section with not yet converted rela data
 */
void convert_rela_section(struct section32 *section) {
    Elf32_Word ranum = section->header.sh_size / sizeof(Elf64_Rela);
    Elf32_Rela *relas32 = malloc(sizeof(Elf32_Rela) * ranum);

    for (Elf32_Word i = 0; i < ranum; i++) {
        Elf64_Rela *rela64 = (Elf64_Rela *)(section->data) + i;
        Elf32_Rela *rela32 = relas32 + i;
        rela32->r_offset = rela64->r_offset;
        rela32->r_addend = rela64->r_addend;

        Elf32_Word sym = ELF64_R_SYM(rela64->r_info);
        Elf64_Xword type64 = ELF64_R_TYPE(rela64->r_info);
        Elf32_Word type32;
        switch (type64) {
            case R_X86_64_32:
            case R_X86_64_32S:
                type32 = R_386_32;
                break;
            case R_X86_64_PC32:
            case R_X86_64_PLT32:
                type32 = R_386_PC32;
                break;
            default:
                type32 = type64;
                warnf("unsupported relocation type: %d\n", type64);
                break;
        }
        rela32->r_info = ELF32_R_INFO(sym, type32);
    }
    free(section->data);
    section->data = relas32;
    section->header.sh_entsize = sizeof(Elf32_Rela);
    section->header.sh_size = sizeof(Elf32_Rela) * ranum;
}

void convert_section32(struct section32 *section, Elf32_Ehdr ehdr32, struct section32 *all_sections) {
    struct section32 shstrtab = all_sections[ehdr32.e_shstrndx];

    char *name = (char *)shstrtab.data + section->header.sh_name;

    if (is_section_symtab(section->header)) {
        printf("converting SYMTAB section: %s\n", name);
        convert_symtab_section(section);
        return;
    }
    if (is_rela_section(section->header)) {
        printf("converting RELA section: %s\n", name);
        convert_rela_section(section);
        return;
    }

    printf("section %s not converted\n", name);
}

// /**
//  * @brief Finds the section header with the given type.
//  * Fails if there is no such section header, or if there are multiple section headers with the given type.
//  *
//  * @param shdrs64 The section header table.
//  * @param shnum The number of section headers in the table.
//  * @param sh_type The type of section header to find.
//  * @return Elf64_Shdr The section header with the given type.
//  */
// Elf64_Shdr find_shdr64(Elf64_Shdr *shdrs64, Elf64_Half shnum, Elf64_Word sh_type) {
//     Elf64_Half index;
//     int found = 0;

//     for (Elf64_Half i = 0; i < shnum; i++) {
//         if (shdrs64[i].sh_type == sh_type) {
//             found++;
//             index = i;
//         }
//     }
//     if (found == 0) {
//         fatalf("no section header with type %d\n", sh_type);
//     } else if (found > 1) {
//         fatalf("multiple (%d) section headers with type %d\n", found, sh_type);
//     }
//     return shdrs64[index];
// }

// Elf64_Shdr find_shdr64_by_name(Elf64_Shdr *shdrs64, Elf64_Half shnum, const char *name, char *shstrtab) {
//     Elf64_Half index;
//     int found = 0;

//     for (Elf64_Half i = 0; i < shnum; i++) {
//         if (strcmp(shstrtab + shdrs64[i].sh_name, name) == 0) {
//             found++;
//             index = i;
//         }
//     }

//     if (found == 0) {
//         fatalf("no section header with name %s\n", name);
//     } else if (found > 1) {
//         fatalf("multiple (%d) section headers with name %s\n", found, name);
//     }
//     return shdrs64[index];
// }

struct section32 *find_section32(const char *name, Elf32_Ehdr ehdr32, struct section32 *sections32) {
    Elf32_Half shnum = ehdr32.e_shnum;
    struct section32 *shstrtab = sections32 + ehdr32.e_shstrndx;
    char *section_names = shstrtab->data;

    Elf32_Half index;
    int found = 0;

    for (Elf32_Half i = 0; i < shnum; i++) {
        if (strcmp(section_names + sections32[i].header.sh_name, name) == 0) {
            found++;
            index = i;
        }
    }

    if (found == 0) {
        fatalf("no section header with name %s\n", name);
    } else if (found > 1) {
        fatalf("multiple (%d) section headers with name %s\n", found, name);
    }

    return sections32 + index;
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
