#include "stubs.h"

#include <elf.h>
#include <string.h>

#include "fatal.h"
#include "funcfile.h"
#include "list.h"

void build_global_stub(Elf32_Word func_sym_index, struct f_func *func_type, elf_file *elf);

void build_exterbal_stub(Elf32_Word func_sym_index, struct f_func *func_type, elf_file *elf);

struct f_func *find_function(char *name, list_t *functions) {
    iterate_list(functions, node) {
        struct f_func *func = list_element(node);
        if (strcmp(func->f_name, name) == 0) {
            return func;
        }
    }
    return NULL;
}

void build_stubs(elf_file *elf, list_t *functions) {
    elf_section *symtab = find_section(".symtab", elf);
    elf_section *strtab = find_section(".strtab", elf);

    Elf32_Word symnum = symtab->s_header.sh_size / symtab->s_header.sh_entsize;
    for (Elf32_Word i = 0; i < symnum; i++) {
        // Get data pointer each time, as add_sym can realloc.
        Elf32_Sym *sym = (Elf32_Sym *)symtab->s_data + i;
        char *name = strtab->s_data + sym->st_name;

        if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC && ELF32_ST_BIND(sym->st_info) == STB_GLOBAL) {
                        struct f_func *f_func = find_function(name, functions);

            if (f_func == NULL) {
                fatalf("Could not find function %s in function file\n", name);
            }

            build_global_stub(i,f_func, elf );
        }
    }

    // TODO: check with function file.
    char *external_function = "doo";

    for (Elf32_Word i = 0; i < symnum; i++) {
        Elf32_Sym *sym = (Elf32_Sym *)symtab->s_data + i;
        char *name = strtab->s_data + sym->st_name;

        if (strcmp(name, external_function) == 0 && ELF32_ST_TYPE(sym->st_info) == STT_NOTYPE && ELF32_ST_BIND(sym->st_info) == STB_GLOBAL) {
            struct f_func *f_func = find_function(name, functions);
            if (f_func == NULL) {
                // symbol is not a function
                continue;
            }

            build_exterbal_stub(i, f_func, elf);
        }
    }
}

char *prefixstr(const char *prefix, char *str) {
    char *newstr = malloc(strlen(prefix) + strlen(str) + 1);
    strcpy(newstr, prefix);
    strcat(newstr, str);
    return newstr;
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

size_t build_stub64_entry(elf_file *elf) {
    return text_push(elf, stub_entry_code, sizeof(stub_entry_code));
}

size_t build_change_32_to_64(elf_file *elf) {
    elf_section *text = find_section(".text", elf);

    // look at char change_32_to_64_code[]
    Elf32_Addr r_offset = text->s_header.sh_size + 2;
    Elf32_Addr lea_target = text->s_header.sh_size + sizeof(change_32_to_64_code);

    Elf32_Rel lea_rel = {
        .r_offset = r_offset,
        .r_info = ELF32_R_INFO(2, R_386_32),  // FIXME: this is hardcoded for now, 2 is .text section symbol
    };
    reltext_push(elf, &lea_rel);

    size_t w = text_push(elf, change_32_to_64_code, sizeof(change_32_to_64_code));
    memcpy(text->s_data + r_offset, &lea_target, sizeof(lea_target));

    return w;
}

size_t build_func64_caller(int func_sym_index, elf_file *elf) {
    elf_section *text = find_section(".text", elf);

    // look at char func64_caller_code[]
    Elf32_Addr r_offset = text->s_header.sh_size + 5;
    Elf32_Sword addend = -4;

    Elf32_Rel func_rel = {
        .r_offset = r_offset,
        .r_info = ELF32_R_INFO(func_sym_index, R_386_PC32),
    };

    reltext_push(elf, &func_rel);
    size_t w = text_push(elf, func64_caller_code, sizeof(func64_caller_code));
    memcpy(text->s_data + r_offset, &addend, sizeof(addend));

    return w;
}

size_t build_change_64_to_32(elf_file *elf) {
    elf_section *text = find_section(".text", elf);

    // look at char change_64_to_32_code[]
    Elf32_Addr r_offset = text->s_header.sh_size + 3;
    Elf32_Addr lea_target = text->s_header.sh_size + sizeof(change_64_to_32_code);

    Elf32_Rel lea_rel = {
        .r_offset = r_offset,
        .r_info = ELF32_R_INFO(2, R_386_32),  // FIXME: this is hardcoded for now, 2 is .text section symbol
    };

    reltext_push(elf, &lea_rel);
    size_t w = text_push(elf, change_64_to_32_code, sizeof(change_64_to_32_code));
    memcpy(text->s_data + r_offset, &lea_target, sizeof(lea_target));

    return w;
}

size_t build_stub64_exit(elf_file *elf) {
    return text_push(elf, stub_exit_code, sizeof(stub_exit_code));
}

/*
This is 64-bit code.

   0:	53                   	push   %rbx
   1:	55                   	push   %rbp
   2:	41 54                	push   %r12
   4:	41 55                	push   %r13
   6:	41 56                	push   %r14
   8:	41 57                	push   %r15
   a:	48 83 ec 08          	sub    $0x8,%rsp
   e:	89 3c 24             	mov    %edi,(%rsp)
*/
char stub32_entry_code[] = {
    0x53,                    // push %rbx
    0x55,                    // push %rbp
    0x41, 0x54,              // push %r12
    0x41, 0x55,              // push %r13
    0x41, 0x56,              // push %r14
    0x41, 0x57,              // push %r15
    0x48, 0x83, 0xec, 0x08,  // sub $0x8,%rsp
    0x89, 0x3c, 0x24,        // mov %edi,(%rsp)
};

/*
This is 32-bit code.

   0:	6a 2b                	push   $0x2b
   2:	1f                   	pop    %ds
   3:	6a 2b                	push   $0x2b
   5:	07                   	pop    %es
   6:	e8 fc ff ff ff       	call   7 <fun_stub_32+0x7>
*/
char func32_caller_code[] = {
    0x6a, 0x2b,                    // push $0x2b
    0x1f,                          // pop %ds
    0x6a, 0x2b,                    // push $0x2b
    0x07,                          // pop %es
    0xe8, 0x00, 0x00, 0x00, 0x00,  // call 7 <fun_stub_32+0x7>
};

/*
This is 64-bit code.

  47:	48 83 c4 08          	add    $0x8,%rsp
  4b:	41 5f                	pop    %r15
  4d:	41 5e                	pop    %r14
  4f:	41 5d                	pop    %r13
  51:	41 5c                	pop    %r12
  53:	5d                   	pop    %rbp
  54:	5b                   	pop    %rbx
  55:	c3                   	retq
*/
char stub32_exit_code[] = {
    0x48, 0x83, 0xc4, 0x08,  // add $0x8,%rsp
    0x41, 0x5f,              // pop %r15
    0x41, 0x5e,              // pop %r14
    0x41, 0x5d,              // pop %r13
    0x41, 0x5c,              // pop %r12
    0x5d,                    // pop %rbp
    0x5b,                    // pop %rbx
    0xc3,                    // retq
};

size_t build_stub32_entry(elf_file *elf) {
    return text_push(elf, stub32_entry_code, sizeof(stub32_entry_code));
}

size_t build_func32_caller(int func_sym_index, elf_file *elf) {
    elf_section *text = find_section(".text", elf);

    // look at char func32_caller_code[]
    Elf32_Addr r_offset = text->s_header.sh_size + 7;
    Elf32_Sword addend = -4;

    Elf32_Rel rel = {
        .r_offset = r_offset,
        .r_info = ELF32_R_INFO(func_sym_index, R_386_PC32),
    };

    reltext_push(elf, &rel);
    size_t w = text_push(elf, func32_caller_code, sizeof(func32_caller_code));
    memcpy(text->s_data + r_offset, &addend, sizeof(addend));

    return w;
}

size_t build_stub32_exit(elf_file *elf) {
    return text_push(elf, stub32_exit_code, sizeof(stub32_exit_code));
}

void build_global_stub(Elf32_Word func_sym_index, struct f_func *func_type, elf_file *elf) {
    elf_section *text = find_section(".text", elf);
    elf_section *strtab = find_section(".strtab", elf);
    elf_section *symtab = find_section(".symtab", elf);

    Elf32_Sym *sym = (Elf32_Sym *)symtab->s_data + func_sym_index;

    Elf32_Word func_name = sym->st_name;
    Elf32_Word stub_start = text->s_header.sh_size;
    size_t stub_size = 0;

    // swap original name with stub name
    char *name = prefixstr("__orig_loc__", strtab->s_data + func_name);
    Elf32_Word name_index = strtab_push(elf, name);
    sym->st_name = name_index;

    stub_size += build_stub64_entry(elf);
    stub_size += build_change_32_to_64(elf);
    stub_size += build_func64_caller(func_sym_index, elf);
    stub_size += build_change_64_to_32(elf);
    stub_size += build_stub64_exit(elf);

    Elf32_Sym stub_sym = {
        .st_name = func_name,
        .st_value = stub_start,
        .st_size = stub_size,
        .st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC),
        .st_other = STV_DEFAULT,
        .st_shndx = sym->st_shndx,  // .text section index
    };
    symtab_push(elf, &stub_sym);
}

void build_exterbal_stub(Elf32_Word func_sym_index, struct f_func *func_type, elf_file *elf) {
    elf_section *text = find_section(".text", elf);
    elf_section *strtab = find_section(".strtab", elf);
    elf_section *symtab = find_section(".symtab", elf);

    // Copy external function symbol
    Elf32_Sym copy_sym = ((Elf32_Sym *)symtab->s_data)[func_sym_index];
    Elf32_Word copy_index = symtab_push(elf, &copy_sym);

    Elf32_Word stub_start = text->s_header.sh_size;
    size_t stub_size = 0;

    stub_size += build_stub32_entry(elf);
    stub_size += build_change_64_to_32(elf);
    stub_size += build_func32_caller(copy_index, elf);
    stub_size += build_change_32_to_64(elf);
    stub_size += build_stub32_exit(elf);

    // Use the original symbol as the stub symbol
    // so relocations will use this one
    Elf32_Sym *stub_sym = (Elf32_Sym *)symtab->s_data + func_sym_index;
    char *stub_name = prefixstr("__stub_ext__", strtab->s_data + stub_sym->st_name);
    Elf32_Word stub_name_index = strtab_push(elf, stub_name);

    stub_sym->st_name = stub_name_index;
    stub_sym->st_value = stub_start;
    stub_sym->st_size = stub_size;
    stub_sym->st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC);
    stub_sym->st_other = STV_DEFAULT;
    stub_sym->st_shndx = 1;  // .text section index FIXME hardcoded
}