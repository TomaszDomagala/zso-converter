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

struct section32 *find_section32(const char *name, Elf32_Ehdr ehdr32, list_t *sections32);

void *load_section64(Elf64_Shdr shdr, FILE *elf64file);

// Elf64_Sym *find_symbol_by_name(Elf64_Sym *syms64, Elf64_Half num_syms, const char *name, char *strtab);
void write_section32_data(FILE *file, struct section32 *section);

void convert_section32(struct section32 *section, Elf32_Ehdr ehdr32, list_t *all_sections);

void create_stubs(Elf32_Ehdr ehdr32, list_t *sections);

void convert_elf(Elf64_Ehdr ehdr64, Elf64_Shdr *shdrs64, FILE *elf64file) {
    // struct section32 *sections;
    // sections = malloc(sizeof(struct section32) * ehdr64.e_shnum);
    list_t *sections = list_create(sizeof(struct section32));

    for (int i = 0; i < ehdr64.e_shnum; i++) {
        struct section32 section;

        Elf64_Shdr shdr = shdrs64[i];
        section.header = initial_convert_shdr(shdr);
        section.data = load_section64(shdr, elf64file);
        list_add(sections, &section);
    }

    Elf32_Ehdr ehdr32 = initial_convert_hdr(ehdr64);

    iterate_list(sections, node) {
        struct section32 *section = list_element(node);
        convert_section32(section, ehdr32, sections);
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

    iterate_list(sections, node) {
        struct section32 *section = list_element(node);
        write_section32_data(newfile, section);
    }

    ehdr32.e_shoff = ftell(newfile);

    iterate_list(sections, node) {
        struct section32 *section = list_element(node);
        Elf32_Shdr shdr = section->header;
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

void rel_add(Elf32_Rel *rel, struct section32 *rel_sec) {
    Elf32_Word new_size = rel_sec->header.sh_size + sizeof(Elf32_Rel);
    rel_sec->data = realloc(rel_sec->data, new_size);
    if (!rel_sec->data) {
        sysfatalf("realloc", "could not reallocate %ld bytes\n", new_size);
    }
    memcpy(rel_sec->data + rel_sec->header.sh_size, rel, sizeof(Elf32_Rel));
    rel_sec->header.sh_size = new_size;
}

/*
This is 32-bit code.

   0:	55                   	push   %ebp
   1:	89 e5                	mov    %esp,%ebp
   3:	53                   	push   %ebx
   4:	57                   	push   %edi
   5:	56                   	push   %esi
*/
char stub_entry_code[] = {
    0x55,        // push %ebp
    0x89, 0xe5,  // mov %esp,%ebp
    0x53,        // push %ebx
    0x57,        // push %edi
    0x56,        // push %esi
};

/*
This is 32-bit code.

   6:	8d 1d 1b 00 00 00    	lea    0x1b,%ebx
   c:	83 ec 08             	sub    $0x8,%esp
   f:	c7 44 24 04 33 00 00 	movl   $0x33,0x4(%esp)
  16:	00
  17:	89 1c 24             	mov    %ebx,(%esp)
  1a:	cb                   	lret

*/
char change_32_to_64_code[] = {
    0x8d, 0x1d, 0x1b, 0x00, 0x00, 0x00,              // lea 0x1b,%ebx
    0x83, 0xec, 0x08,                                // sub $0x8,%esp
    0xc7, 0x44, 0x24, 0x04, 0x33, 0x00, 0x00, 0x00,  // movl $0x33,0x4(%esp)
    0x89, 0x1c, 0x24,                                // mov %ebx,(%esp)
    0xcb,                                            // lret
};

void build_stub_entry(struct global_func *func, Elf32_Ehdr ehdr32, list_t *sections) {
    struct section32 *text = find_section32(".text", ehdr32, sections);
    struct section32 *strtab = find_section32(".strtab", ehdr32, sections);
    struct section32 *symtab = find_section32(".symtab", ehdr32, sections);

    char *name = prefixstr("stub_entry_", strtab->data + func->sym->st_name);
    Elf32_Word name_index = strtab_add(name, strtab);

    Elf32_Sym entry_sym = {
        .st_name = name_index,
        .st_value = text->header.sh_size,
        .st_size = sizeof(stub_entry_code) + sizeof(change_32_to_64_code),
        .st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
        .st_other = STV_DEFAULT,
        .st_shndx = func->sym->st_shndx,  // .text section index
    };

    symtab_add(&entry_sym, symtab);
    text_add(stub_entry_code, sizeof(stub_entry_code), text);
}

void build_jump_32_to_64(struct global_func *func, Elf32_Ehdr ehdr32, list_t *sections) {
    struct section32 *text = find_section32(".text", ehdr32, sections);
    struct section32 *reltext = find_section32(".rel.text", ehdr32, sections);

    // look at char change_32_to_64_code[]
    Elf32_Addr r_offset = text->header.sh_size + 2;
    Elf32_Addr lea_target = text->header.sh_size + sizeof(change_32_to_64_code);

    Elf32_Rel lea_rel = {
        .r_offset = r_offset,
        .r_info = ELF32_R_INFO(2, R_386_32),  // FIXME: this is hardcoded for now, 2 is .text section symbol
    };
    rel_add(&lea_rel, reltext);

    text_add(change_32_to_64_code, sizeof(change_32_to_64_code), text);
    memcpy(text->data + r_offset, &lea_target, sizeof(lea_target));
}

/*
This is 64-bit code.

  18:	67 8b 7d 08          	mov    0x8(%ebp),%edi
  1c:	e8 00 00 00 00       	callq  21 <fun_stub_64+0x9>
  21:	48 89 c2             	mov    %rax,%rdx
  24:	48 c1 ea 20          	shr    $0x20,%rdx
*/
char func64_caller_code[] = {
    0x67, 0x8b, 0x7d, 0x08,        // mov 0x8(%ebp),%edi
    0xe8, 0x00, 0x00, 0x00, 0x00,  // callq fun_stub_64+0x9
    0x48, 0x89, 0xc2,              // mov %rax,%rdx
    0x48, 0xc1, 0xea, 0x20,        // shr $0x20,%rdx
};

/*
This is 64-bit code.

  2b:	8d 1c 25 00 00 00 00 	lea    0x0,%ebx
  32:	83 ec 08             	sub    $0x8,%esp
  35:	c7 44 24 04 23 00 00 	movl   $0x23,0x4(%rsp)
  3c:	00 
  3d:	89 1c 24             	mov    %ebx,(%rsp)
  40:	cb                   	lret   
*/
char change_64_to_32_code[] = {
    0x8d, 0x1c, 0x25, 0x00, 0x00, 0x00, 0x00,        // lea 0x0,%ebx
    0x83, 0xec, 0x08,                                // sub $0x8,%esp
    0xc7, 0x44, 0x24, 0x04, 0x23, 0x00, 0x00, 0x00,  // movl $0x23,0x4(%rsp)
    0x89, 0x1c, 0x24,                                // mov %ebx,(%rsp)
    0xcb,                                            // lret
};

void build_func64_caller(struct global_func *func, Elf32_Ehdr ehdr32, list_t *sections) {
    struct section32 *text = find_section32(".text", ehdr32, sections);
    struct section32 *reltext = find_section32(".rel.text", ehdr32, sections);
    struct section32 *strtab = find_section32(".strtab", ehdr32, sections);
    struct section32 *symtab = find_section32(".symtab", ehdr32, sections);

    char *name = prefixstr("func_caller_", strtab->data + func->sym->st_name);
    Elf32_Word name_index = strtab_add(name, strtab);

    Elf32_Sym caller_sym = {
        .st_name = name_index,
        .st_value = text->header.sh_size,
        .st_size = sizeof(func64_caller_code) + sizeof(change_64_to_32_code),
        .st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
        .st_other = STV_DEFAULT,
        .st_shndx = func->sym->st_shndx,  // .text section index
    };
    symtab_add(&caller_sym, symtab);

    // look at char func64_caller_code[]
    Elf32_Addr r_offset = text->header.sh_size + 5;
    Elf32_Sword addend = -4;

    Elf32_Rel func_rel = {
        .r_offset = r_offset,
        .r_info = ELF32_R_INFO(func->sym_index, R_386_PC32),
    };

    rel_add(&func_rel, reltext);
    text_add(func64_caller_code, sizeof(func64_caller_code), text);
    memcpy(text->data + r_offset, &addend, sizeof(addend));
}

void build_change_64_to_32(struct global_func *func, Elf32_Ehdr ehdr32, list_t *sections) {
    struct section32 *text = find_section32(".text", ehdr32, sections);
    struct section32 *reltext = find_section32(".rel.text", ehdr32, sections);

    // look at char change_64_to_32_code[]
    Elf32_Addr r_offset = text->header.sh_size + 3;
    Elf32_Addr lea_target = text->header.sh_size + sizeof(change_64_to_32_code);

    Elf32_Rel lea_rel = {
        .r_offset = r_offset,
        .r_info = ELF32_R_INFO(2, R_386_32),  // FIXME: this is hardcoded for now, 2 is .text section symbol
    };

    rel_add(&lea_rel, reltext);
    text_add(change_64_to_32_code, sizeof(change_64_to_32_code), text);
    memcpy(text->data + r_offset, &lea_target, sizeof(lea_target));
}

/*
This is 32-bit code.

  18:	5e                   	pop    %esi
  19:	5f                   	pop    %edi
  1a:	5b                   	pop    %ebx
  1b:	5d                   	pop    %ebp
  1c:	c3                   	ret
*/
char stub_exit_code[] = {
    0x5e,  // pop %esi
    0x5f,  // pop %edi
    0x5b,  // pop %ebx
    0x5d,  // pop %ebp
    0xc3,  // ret
};

void build_stub_exit(struct global_func *func, Elf32_Ehdr ehdr32, list_t *sections) {
    struct section32 *text = find_section32(".text", ehdr32, sections);
    struct section32 *strtab = find_section32(".strtab", ehdr32, sections);
    struct section32 *symtab = find_section32(".symtab", ehdr32, sections);

    char *name = prefixstr("stub_exit_", strtab->data + func->sym->st_name);
    Elf32_Word name_index = strtab_add(name, strtab);

    Elf32_Sym stub_exit_sym = {
        .st_name = name_index,
        .st_value = text->header.sh_size,
        .st_size = sizeof(stub_exit_code),
        .st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
        .st_other = STV_DEFAULT,
        .st_shndx = func->sym->st_shndx,  // .text section index
    };
    symtab_add(&stub_exit_sym, symtab);
    text_add(stub_exit_code, sizeof(stub_exit_code), text);
}

void create_function_stubs(struct global_func *func, Elf32_Ehdr ehdr32, list_t *sections) {
    build_stub_entry(func, ehdr32, sections);
    build_jump_32_to_64(func, ehdr32, sections);
    build_func64_caller(func, ehdr32, sections);
    build_change_64_to_32(func, ehdr32, sections);
    build_stub_exit(func, ehdr32, sections);

    // struct section32 *text = find_section32(".text", ehdr32, sections);
    // struct section32 *reltext = find_section32(".rel.text", ehdr32, sections);
    // struct section32 *strtab = find_section32(".strtab", ehdr32, sections);
    // struct section32 *symtab = find_section32(".symtab", ehdr32, sections);

    // char *stub_in_name = prefixstr("stub_", strtab->data + func->sym->st_name);
    // char *stub_out_name = prefixstr("stub_out_", strtab->data + func->sym->st_name);

    // Elf32_Word stub_in_name_index = strtab_add(stub_in_name, strtab);
    // Elf32_Word stub_out_name_index = strtab_add(stub_out_name, strtab);

    // char *stub_caller_name = prefixstr("stub_caller_", strtab->data + func->sym->st_name);
    // Elf32_Word stub_caller_index = strtab_add(stub_caller_name, strtab);

    // Elf32_Sym stub_in_sym = {
    //     .st_name = stub_in_name_index,
    //     .st_value = text->header.sh_size,
    //     .st_size = sizeof(fun_stub),
    //     .st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
    //     .st_other = STV_DEFAULT,
    //     .st_shndx = func->sym->st_shndx,  // .text section index
    // };

    // Elf32_Sym stub_caller_sym = {
    //     .st_name = stub_caller_index,
    //     .st_value = text->header.sh_size + sizeof(fun_stub),
    //     .st_size = sizeof(fun_stub_64),
    //     .st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
    //     .st_other = STV_DEFAULT,
    //     .st_shndx = func->sym->st_shndx,  // .text section index
    // };

    // Elf32_Sym stub_out_sym = {
    //     .st_name = stub_out_name_index,
    //     .st_value = text->header.sh_size + sizeof(fun_stub) + sizeof(fun_stub_64),
    //     .st_size = sizeof(fun_stub_out),
    //     .st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
    //     .st_other = STV_DEFAULT,
    //     .st_shndx = func->sym->st_shndx,  // .text section index
    // };

    // symtab_add(&stub_in_sym, symtab);
    // symtab_add(&stub_caller_sym, symtab);
    // symtab_add(&stub_out_sym, symtab);

    // Elf32_Rel caller_rel = {
    //     .r_offset = text->header.sh_size + 0xe,
    //     .r_info = ELF32_R_INFO(2, R_386_32),
    // };

    // rel_add(&caller_rel, reltext);

    // text_add(fun_stub, sizeof(fun_stub), text);
    // Elf32_Addr lea_addr = text->header.sh_size;
    // memcpy(text->data + text->header.sh_size - sizeof(fun_stub) + 0xe, &lea_addr, sizeof(Elf32_Addr));

    // Elf32_Rel original_func_rel = {
    //     .r_offset = text->header.sh_size + 0x5,
    //     .r_info = ELF32_R_INFO(func->sym_index, R_386_PC32),
    // };
    // Elf32_Rel out_rel = {
    //     .r_offset = text->header.sh_size + 19,
    //     .r_info = ELF32_R_INFO(2, R_386_32),  // TODO: this is hardcoded, 2 is symbol related to .text
    // };

    // rel_add(&original_func_rel, reltext);
    // rel_add(&out_rel, reltext);

    // text_add(fun_stub_64, sizeof(fun_stub_64), text);
    // Elf32_Sword addend = -4;
    // memcpy(text->data + text->header.sh_size - sizeof(fun_stub_64) + 0x5, &addend, sizeof(Elf32_Sword));
    // Elf32_Addr lea_addr_64 = text->header.sh_size;
    // memcpy(text->data + text->header.sh_size - sizeof(fun_stub_64) + 19, &lea_addr_64, sizeof(Elf32_Addr));

    // text_add(fun_stub_out, sizeof(fun_stub_out), text);

    // return;
    // struct section32 *text = find_section32(".text", ehdr32, sections);
    // struct section32 *reltext = find_section32(".rel.text", ehdr32, sections);
    // struct section32 *strtab = find_section32(".strtab", ehdr32, sections);
    // struct section32 *symtab = find_section32(".symtab", ehdr32, sections);

    // char *stub_in_name = prefixstr("stub_", strtab->data + func->sym->st_name);
    // // char *stub_caller = prefixstr("caller_", strtab->data + func->sym->st_name);
    // char *stub_out_name = prefixstr("stub_out_", strtab->data + func->sym->st_name);

    // Elf32_Word stub_in_name_index = strtab_add(stub_in_name, strtab);
    // // Elf32_Word stub_caller_index = strtab_add(stub_caller, strtab);
    // Elf32_Word stub_out_name_index = strtab_add(stub_out_name, strtab);

    // // Elf32_Word stub_out_namendx = strtab_add(stub_out_name, strtab);

    // Elf32_Sym stub_in_sym = {
    //     .st_name = stub_in_name_index,
    //     .st_value = text->header.sh_size,
    //     .st_size = sizeof(fun_stub),
    //     .st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
    //     .st_other = STV_DEFAULT,
    //     .st_shndx = func->sym->st_shndx,  // .text section index
    // };

    // // Elf32_Sym stub_caller_sym = {
    // //     .st_name = stub_caller_index,
    // //     .st_value = text->header.sh_size + sizeof(fun_stub),
    // //     .st_size = sizeof(fun_stub_64),
    // //     .st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
    // //     .st_other = STV_DEFAULT,
    // //     .st_shndx = func->sym->st_shndx,  // .text section index
    // // };

    // Elf32_Sym stub_out_sym = {
    //     .st_name = stub_out_name_index,
    //     .st_value = text->header.sh_size + sizeof(fun_stub) + sizeof(fun_stub_64),
    //     .st_size = sizeof(fun_stub_out),
    //     .st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
    //     .st_other = STV_DEFAULT,
    //     .st_shndx = func->sym->st_shndx,  // .text section index
    // };

    // symtab_add(&stub_in_sym, symtab);
    // // symtab_add(&stub_caller_sym, symtab);
    // symtab_add(&stub_out_sym, symtab);

    // Elf32_Rel caller_rel = {
    //     .r_offset = text->header.sh_size + 0xe,
    //     .r_info = ELF32_R_INFO(2, R_386_32),  // TODO: this is hardcoded, 2 is symbol related to .text
    // };

    // rel_add(&caller_rel, reltext);

    // text_add(fun_stub, sizeof(fun_stub), text);
    // Elf32_Addr lea_addr = text->header.sh_size + sizeof(fun_stub);
    // memcpy(text->data + text->header.sh_size - sizeof(fun_stub) + 0xe, &lea_addr, sizeof(Elf32_Addr));

    // // Elf32_Rel original_func_rel = {
    // //     .r_offset = text->header.sh_size + 0x5,
    // //     .r_info = ELF32_R_INFO(func->sym_index, R_386_PC32),
    // // };
    // // Elf32_Rel out_rel = {
    // //     .r_offset = text->header.sh_size + 19,
    // //     .r_info = ELF32_R_INFO(2, R_386_32),  // TODO: this is hardcoded, 2 is symbol related to .text
    // // };

    // // rel_add(&original_func_rel, reltext);
    // // rel_add(&out_rel, reltext);

    // text_add(fun_stub_64, sizeof(fun_stub_64), text);
    // // Elf32_Sword addend = -4;
    // // memcpy(text->data + text->header.sh_size - sizeof(fun_stub_64) + 0x5, &addend, sizeof(Elf32_Sword));
    // // Elf32_Addr lea_addr_64 = text->header.sh_size;
    // // memcpy(text->data + text->header.sh_size - sizeof(fun_stub_64) + 19, &lea_addr_64, sizeof(Elf32_Addr));

    // text_add(fun_stub_out, sizeof(fun_stub_out), text);
}

void create_stubs(Elf32_Ehdr ehdr32, list_t *sections) {
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

// /**
//  * @brief Check if a section is a RELA table
//  *
//  * @param shdr section header
//  * @return true if section is a RELA table
//  * @return false if section is not a RELA table
//  */
// bool is_rela_section(Elf32_Shdr shdr) {
//     return shdr.sh_type == SHT_RELA;
// }

// /**
//  * @brief Converts Elf64_Rela to Elf32_Rela and supported relocation types.
//  * This function expects section to be initiali converted section with untouched data
//  *
//  * @param section 32-bit rela section with not yet converted rela data
//  */
// void convert_rela_section(struct section32 *section, list_t *all_sections) {
//     Elf32_Word ranum = section->header.sh_size / sizeof(Elf64_Rela);
//     Elf32_Rela *relas32 = malloc(sizeof(Elf32_Rela) * ranum);

//     for (Elf32_Word i = 0; i < ranum; i++) {
//         Elf64_Rela *rela64 = (Elf64_Rela *)(section->data) + i;
//         Elf32_Rela *rela32 = relas32 + i;
//         rela32->r_offset = rela64->r_offset;
//         rela32->r_addend = rela64->r_addend;

//         Elf32_Word sym = ELF64_R_SYM(rela64->r_info);
//         Elf64_Xword type64 = ELF64_R_TYPE(rela64->r_info);
//         Elf32_Word type32;
//         switch (type64) {
//             case R_X86_64_32:
//             case R_X86_64_32S:
//                 type32 = R_386_32;
//                 break;
//             case R_X86_64_PC32:
//             case R_X86_64_PLT32:
//                 type32 = R_386_PC32;
//                 break;
//             default:
//                 type32 = type64;
//                 warnf("unsupported relocation type: %d\n", type64);
//                 break;
//         }
//         rela32->r_info = ELF32_R_INFO(sym, type32);
//     }
//     free(section->data);
//     section->data = relas32;
//     section->header.sh_entsize = sizeof(Elf32_Rela);
//     section->header.sh_size = sizeof(Elf32_Rela) * ranum;
// }

struct section32 *find_shstrtab(Elf32_Ehdr ehdr32, list_t *sections32) {
    struct section32 *shstrtab = NULL;
    int i = 0;

    iterate_list(sections32, node) {
        if (i == ehdr32.e_shstrndx) {
            shstrtab = list_element(node);
            break;
        }
        i++;
    }
    if (!shstrtab) {
        fatal("could not find section header string table\n");
    }
    if (shstrtab->header.sh_type != SHT_STRTAB) {
        fatal("section header string table is not a string table\n");
    }
    return shstrtab;
}

void adjust_addend(struct section32 *text, Elf32_Addr r_offset, Elf32_Sword r_addend) {
    memcpy(text->data + r_offset, &r_addend, sizeof(Elf32_Sword));
}

bool is_rela_text_section(struct section32 *section, struct section32 *shstrtab) {
    return strcmp((char *)shstrtab->data + section->header.sh_name, ".rela.text") == 0;
}

void convert_rela_text(struct section32 *relatext, Elf32_Ehdr ehdr32, list_t *all_sections) {
    struct section32 *shstrtab = find_shstrtab(ehdr32, all_sections);
    // replace .rela.text string with .rel.text string
    // actually, replacing prefix .rela with \0.rel and changing sh_name
    // is necessary, as .text section can use .rela.text substring as name
    memset((char *)shstrtab->data + relatext->header.sh_name, 0, strlen(".rela"));
    memcpy((char *)shstrtab->data + relatext->header.sh_name, "\0.rel", 1 + strlen(".rel"));
    relatext->header.sh_name++;

    struct section32 *text = find_section32(".text", ehdr32, all_sections);

    Elf32_Word rel_num = relatext->header.sh_size / sizeof(Elf64_Rela);
    Elf32_Shdr rel_text_shdr;

    rel_text_shdr.sh_name = relatext->header.sh_name;  // now changed to .rel.text
    rel_text_shdr.sh_type = SHT_REL;
    rel_text_shdr.sh_flags = relatext->header.sh_flags;
    rel_text_shdr.sh_addr = relatext->header.sh_addr;
    rel_text_shdr.sh_offset = relatext->header.sh_offset;
    rel_text_shdr.sh_size = sizeof(Elf32_Rel) * rel_num;
    rel_text_shdr.sh_link = relatext->header.sh_link;
    rel_text_shdr.sh_info = relatext->header.sh_info;
    rel_text_shdr.sh_addralign = relatext->header.sh_addralign;
    rel_text_shdr.sh_entsize = sizeof(Elf32_Rel);

    Elf32_Rel *rels = malloc(rel_text_shdr.sh_size);
    if (!rels) {
        fatal("could not allocate memory for rel.text section\n");
    }

    for (Elf32_Word i = 0; i < rel_num; i++) {
        Elf64_Rela *rela64 = (Elf64_Rela *)(relatext->data) + i;
        Elf32_Rel *rel32 = rels + i;
        rel32->r_offset = rela64->r_offset;

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
        rel32->r_info = ELF32_R_INFO(sym, type32);
        adjust_addend(text, rel32->r_offset, rela64->r_addend);
    }
    free(relatext->data);
    relatext->data = rels;
    relatext->header = rel_text_shdr;
}

void convert_section32(struct section32 *section, Elf32_Ehdr ehdr32, list_t *all_sections) {
    struct section32 *shstrtab = find_shstrtab(ehdr32, all_sections);

    char *name = (char *)shstrtab->data + section->header.sh_name;

    if (is_section_symtab(section->header)) {
        printf("converting SYMTAB section: %s\n", name);
        convert_symtab_section(section);
        return;
    }
    if (is_rela_text_section(section, shstrtab)) {
        printf("converting .rela.text section: %s\n", name);
        convert_rela_text(section, ehdr32, all_sections);
        return;
    }
    // if (is_rela_section(section->header)) {
    //     printf("converting RELA section: %s\n", name);
    //     convert_rela_section(section);
    //     return;
    // }

    printf("section %s not converted\n", name);
}

struct section32 *find_section32(const char *name, Elf32_Ehdr ehdr32, list_t *sections32) {
    struct section32 *shstrtab = find_shstrtab(ehdr32, sections32);
    char *section_names = shstrtab->data;

    struct section32 *wanted;
    int found = 0;

    iterate_list(sections32, node) {
        struct section32 *section = list_element(node);
        if (strcmp(section_names + section->header.sh_name, name) == 0) {
            found++;
            wanted = section;
        }
    }

    if (found == 0) {
        fatalf("no section header with name %s\n", name);
    } else if (found > 1) {
        fatalf("multiple (%d) section headers with name %s\n", found, name);
    }

    return wanted;
}

// Elf64_Sym *find_symbol_by_name(Elf64_Sym *syms64, Elf64_Half num_syms, const char *name, char *strtab) {
//     Elf64_Half index;
//     int found = 0;

//     for (Elf64_Half i = 0; i < num_syms; i++) {
//         if (strcmp(strtab + syms64[i].st_name, name) == 0) {
//             found++;
//             index = i;
//         }
//     }

//     if (found == 0) {
//         fatalf("no symbol with name %s\n", name);
//     } else if (found > 1) {
//         fatalf("multiple (%d) symbols with name %s\n", found, name);
//     }
//     return syms64 + index;
// }

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
