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

    // TODO remove this
    struct section32 *eh_frame = find_section32(".eh_frame", ehdr32, sections);
    memset(&eh_frame->header, 0, sizeof(Elf32_Shdr));
    struct section32 *rela_eh_frame = find_section32(".rela.eh_frame", ehdr32, sections);
    memset(&rela_eh_frame->header, 0, sizeof(Elf32_Shdr));

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
// char fun_stub[] = {
//     0x55,                                      // push %ebp
//     0x89, 0xe5,                                // mov %esp,%ebp
//     0x53,                                      // push %ebx
//     0x57,                                      // push %edi
//     0x56,                                      // push %esi
//     0x83, 0xec, 0x04,                          // sub $0x4,%esp
//     0x83, 0xec, 0x08,                          // sub $0x8,%esp
//     0x8d, 0x1d, 0x00, 0x00, 0x00, 0x00,        // lea 0x1e,%ebx
//     0xc7, 0x44, 0x24, 0x04, 0x23, 0x00, 0x00,  // movl $0x23,0x4(%esp)
//     0x00,
//     0x89, 0x1c, 0x24,  // mov %ebx,(%esp)
//     0xcb,              // lret
// };

// char fun_stub_out[] = {
//     0x8b, 0x45, 0x08,  // mov 0x8(%ebp),%eax
//     0x83, 0xc4, 0x04,  // add $0x4,%esp
//     0x5e,              // pop %esi
//     0x5f,              // pop %edi
//     0x5b,              // pop %ebx
//     0x5d,              // pop %ebp
//     0xc3,              // ret
// };

/*
00000029 <test>:
test-32.o:     file format elf32-i386


Disassembly of section .text:

00000000 <foo>:
   0:	55                   	push   %ebp
   1:	89 e5                	mov    %esp,%ebp
   3:	83 ec 10             	sub    $0x10,%esp
   6:	8b 45 08             	mov    0x8(%ebp),%eax
   9:	01 c0                	add    %eax,%eax
   b:	89 45 fc             	mov    %eax,-0x4(%ebp)
   e:	8b 45 fc             	mov    -0x4(%ebp),%eax
  11:	c9                   	leave
  12:	c3                   	ret

00000013 <bar>:
  13:	55                   	push   %ebp
  14:	89 e5                	mov    %esp,%ebp
  16:	83 ec 10             	sub    $0x10,%esp
  19:	ff 75 08             	pushl  0x8(%ebp)
  1c:	e8 fc ff ff ff       	call   1d <bar+0xa>
  21:	83 c4 04             	add    $0x4,%esp
  24:	8b 55 08             	mov    0x8(%ebp),%edx
  27:	01 d0                	add    %edx,%eax
  29:	89 45 fc             	mov    %eax,-0x4(%ebp)
  2c:	8b 45 fc             	mov    -0x4(%ebp),%eax
  2f:	c9                   	leave
  30:	c3                   	ret
*/
char foo_stub[] = {
    0x55,              // push %ebp
    0x89, 0xe5,        // mov %esp,%ebp
    0x83, 0xec, 0x10,  // sub $0x10,%esp
    0x8b, 0x45, 0x08,  // mov 0x8(%ebp),%eax
    0x01, 0xc0,        // add %eax,%eax
    0x89, 0x45, 0xfc,  // mov %eax,-0x4(%ebp)
    0x8b, 0x45, 0xfc,  // mov -0x4(%ebp),%eax
    0xc9,              // leave
    0xc3,              // ret
};

char bar_stub[] = {
    0x55,                          // push %ebp
    0x89, 0xe5,                    // mov %esp,%ebp
    0x83, 0xec, 0x10,              // sub $0x10,%esp
    0xff, 0x75, 0x08,              // pushl  0x8(%ebp)
    0xe8, 0x00, 0x00, 0x00, 0x00,  // call   1d <bar+0xa>
    0x83, 0xc4, 0x04,              // add    $0x4,%esp
    0x8b, 0x55, 0x08,              // mov    0x8(%ebp),%edx
    0x01, 0xd0,                    // add    %edx,%eax
    0x89, 0x45, 0xfc,              // mov    %eax,-0x4(%ebp)
    0x8b, 0x45, 0xfc,              // mov    -0x4(%ebp),%eax
    0xc9,                          // leave
    0xc3,                          // ret
};

struct global_func {
    Elf32_Sym *sym;
    Elf32_Word sym_index;
};

char *prefixstr(const char *prefix, char *str) {
    char *newstr = malloc(strlen(prefix) + strlen(str) + 1);
    strcpy(newstr, prefix);
    strcat(newstr, str);
    return newstr;
}

/**
 * @brief Adds string to the strtab section.
 *
 * @param str string to add
 * @param strtab section to add string to
 * @return Elf32_Word index of string in strtab
 */
Elf32_Word strtab_add(char *str, struct section32 *strtab) {
    Elf32_Word new_size = strtab->header.sh_size + strlen(str) + 1;
    strtab->data = realloc(strtab->data, new_size);
    if (!strtab->data) {
        sysfatalf("realloc", "could not reallocate %ld bytes\n", new_size);
    }
    Elf32_Word index = strtab->header.sh_size;

    strcpy((char *)strtab->data + index, str);
    strtab->header.sh_size = new_size;

    return index;
}

/**
 * @brief Adds symbol to the symtab section.
 *
 * @param sym symbol to add
 * @param symtab section to add symbol to
 * @return Elf32_Word index of symbol in symtab
 */
Elf32_Word symtab_add(Elf32_Sym *sym, struct section32 *symtab) {
    Elf32_Word new_size = symtab->header.sh_size + sizeof(Elf32_Sym);
    symtab->data = realloc(symtab->data, new_size);
    if (!symtab->data) {
        sysfatalf("realloc", "could not reallocate %ld bytes\n", new_size);
    }
    memcpy(symtab->data + symtab->header.sh_size, sym, sizeof(Elf32_Sym));
    symtab->header.sh_size = new_size;

    return symtab->header.sh_size / sizeof(Elf32_Sym) - 1;
}

void text_add(char *code, size_t size, struct section32 *text) {
    Elf32_Word new_size = text->header.sh_size + size;
    text->data = realloc(text->data, new_size);
    if (!text->data) {
        sysfatalf("realloc", "could not reallocate %ld bytes\n", new_size);
    }
    memcpy(text->data + text->header.sh_size, code, size);
    text->header.sh_size = new_size;
}

void rela_add(Elf32_Rela *rela, struct section32 *rela_sec) {
    Elf32_Word new_size = rela_sec->header.sh_size + sizeof(Elf32_Rela);
    rela_sec->data = realloc(rela_sec->data, new_size);
    if (!rela_sec->data) {
        sysfatalf("realloc", "could not reallocate %ld bytes\n", new_size);
    }
    memcpy(rela_sec->data + rela_sec->header.sh_size, rela, sizeof(Elf32_Rela));
    rela_sec->header.sh_size = new_size;
}

void create_function_stubs(struct global_func *func, Elf32_Ehdr ehdr32, struct section32 *sections) {
    struct section32 *text = find_section32(".text", ehdr32, sections);
    struct section32 *relatext = find_section32(".rela.text", ehdr32, sections);
    struct section32 *strtab = find_section32(".strtab", ehdr32, sections);
    struct section32 *symtab = find_section32(".symtab", ehdr32, sections);

    // char *stub_in_name = prefixstr("stub_", strtab->data + func->sym->st_name);
    // char *stub_out_name = prefixstr("stub_out_", strtab->data + func->sym->st_name);
    // printf("%s\n", stub_in_name);
    // printf("%s\n", stub_out_name);

    char *foo_stub = prefixstr("foo_stub___", strtab->data + func->sym->st_name);
    char *bar_stub = prefixstr("bar_stub___", strtab->data + func->sym->st_name);

    Elf32_Word stub_foo_index = strtab_add(foo_stub, strtab);
    Elf32_Word stub_bar_index = strtab_add(bar_stub, strtab);

    // Elf32_Word stub_out_namendx = strtab_add(stub_out_name, strtab);

    Elf32_Sym stub_foo_sym = {
        .st_name = stub_foo_index,
        .st_value = text->header.sh_size,
        .st_size = sizeof(foo_stub),
        .st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
        .st_other = STV_DEFAULT,
        .st_shndx = func->sym->st_shndx,  // .text section index
    };

    Elf32_Sym stub_bar_sym = {
        .st_name = stub_bar_index,
        .st_value = text->header.sh_size + sizeof(foo_stub),
        .st_size = sizeof(bar_stub),
        .st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
        .st_other = STV_DEFAULT,
        .st_shndx = func->sym->st_shndx,  // .text section index
    };

    Elf32_Word stub_foo_sym_index = symtab_add(&stub_foo_sym, symtab);
    Elf32_Word stub_bar_sym_index = symtab_add(&stub_bar_sym, symtab);

    Elf32_Rela foo_rela = {
        .r_offset = text->header.sh_size + 0x1d,
        .r_info = ELF32_R_INFO(stub_foo_sym_index, R_386_PC32),
        .r_addend = -4,
    };

    rela_add(&foo_rela, relatext);

    text_add(foo_stub, sizeof(foo_stub), text);
    text_add(bar_stub, sizeof(bar_stub), text);
    // text_add(fun_stub_out, sizeof(fun_stub_out), text);
}

void create_stubs(Elf32_Ehdr ehdr32, struct section32 *sections) {
    struct section32 *symtab = find_section32(".symtab", ehdr32, sections);
    struct section32 *strtab = find_section32(".strtab", ehdr32, sections);

    Elf32_Sym *syms = symtab->data;
    // list of global functions
    list_t *funcs = list_create(sizeof(struct global_func));

    Elf32_Word symnum = symtab->header.sh_size / symtab->header.sh_entsize;
    for (Elf32_Word i = 0; i < symnum; i++) {
        Elf32_Sym *sym = &syms[i];

        if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC && ELF32_ST_BIND(sym->st_info) == STB_GLOBAL) {
            struct global_func func = {
                .sym = sym,
                .sym_index = i,
            };

            list_add(funcs, &func);
        }
    }

    iterate_list(funcs, node) {
        struct global_func *func = list_element(node);

        printf("function name: %s\n", (char *)strtab->data + func->sym->st_name);
        printf("sym_index: %d\n", func->sym_index);

        create_function_stubs(func, ehdr32, sections);
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
