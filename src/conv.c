#include "conv.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "fatal.h"
#include "list.h"

Elf32_Ehdr convert_elf_header(Elf64_Ehdr ehdr64) {
    Elf32_Ehdr ehdr32;
    memcpy(ehdr32.e_ident, ehdr64.e_ident, sizeof(ehdr32.e_ident));
    ehdr32.e_ident[EI_CLASS] = ELFCLASS32;
    ehdr32.e_type = ET_REL;
    ehdr32.e_machine = EM_386;
    ehdr32.e_version = EV_CURRENT;
    ehdr32.e_entry = 0;
    ehdr32.e_phoff = 0;
    ehdr32.e_shoff = (Elf32_Off)ehdr64.e_shoff;
    ehdr32.e_flags = ehdr64.e_flags;
    ehdr32.e_ehsize = (Elf32_Half)52;
    ehdr32.e_phentsize = 0;
    ehdr32.e_phnum = 0;
    ehdr32.e_shentsize = sizeof(Elf32_Shdr);
    ehdr32.e_shnum = ehdr64.e_shnum;
    ehdr32.e_shstrndx = ehdr64.e_shstrndx;

    return ehdr32;
}

Elf32_Shdr convert_section_header(Elf64_Shdr shdr64) {
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

/**
 * @brief Converts Elf64_Sym to Elf32_Sym
 * This function expects section to be initially converted section with untouched data
 * containing Elf64_Sym table.
 * @param section 32-bit symtab section with not yet converted symbols data
 */
void convert_symtab(elf_section* symtab) {
    Elf32_Word snum = symtab->s_header.sh_size / sizeof(Elf64_Sym);
    Elf32_Sym* syms32 = malloc(sizeof(Elf32_Sym) * snum);

    for (Elf32_Word i = 0; i < snum; i++) {
        Elf64_Sym* sym64 = (Elf64_Sym*)(symtab->s_data) + i;
        Elf32_Sym* sym32 = syms32 + i;
        sym32->st_name = sym64->st_name;
        sym32->st_value = sym64->st_value;
        sym32->st_size = sym64->st_size;
        // could use ELF32_ST_INFO and friends, but st_info for both 32 and 64 bit is the same.
        sym32->st_info = sym64->st_info;
        sym32->st_other = sym64->st_other;
        sym32->st_shndx = sym64->st_shndx;
    }
    free(symtab->s_data);
    symtab->s_data = syms32;
    symtab->s_header.sh_entsize = sizeof(Elf32_Sym);
    symtab->s_header.sh_size = sizeof(Elf32_Sym) * snum;
}

void adjust_addend(elf_section* text, Elf32_Addr r_offset, Elf32_Sword r_addend) {
    memcpy(text->s_data + r_offset, &r_addend, sizeof(Elf32_Sword));
}

void convert_rela(elf_section* rela_section) {
    elf_file* elf = rela_section->s_elf;
    elf_section* shstrtab = find_section(".shstrtab", elf);

    // replace .rela.foo string with .rel.foo string
    // actually, replacing prefix .rela with \0.rel and changing sh_name
    // is necessary, as .foo section can use .rela.foo substring as its name
    memset((char*)shstrtab->s_data + rela_section->s_header.sh_name, 0, strlen(".rela"));
    memcpy((char*)shstrtab->s_data + rela_section->s_header.sh_name, "\0.rel", 1 + strlen(".rel"));
    rela_section->s_header.sh_name++;

    Elf32_Word rel_num = rela_section->s_header.sh_size / sizeof(Elf64_Rela);
    Elf32_Shdr relsec_shdr;

    relsec_shdr.sh_name = rela_section->s_header.sh_name;  // now changed to .rel.foo
    relsec_shdr.sh_type = SHT_REL;
    relsec_shdr.sh_flags = rela_section->s_header.sh_flags;
    relsec_shdr.sh_addr = rela_section->s_header.sh_addr;
    relsec_shdr.sh_offset = rela_section->s_header.sh_offset;
    relsec_shdr.sh_size = sizeof(Elf32_Rel) * rel_num;
    relsec_shdr.sh_link = rela_section->s_header.sh_link;
    relsec_shdr.sh_info = rela_section->s_header.sh_info;
    relsec_shdr.sh_addralign = rela_section->s_header.sh_addralign;
    relsec_shdr.sh_entsize = sizeof(Elf32_Rel);

    Elf32_Rel* rels = malloc(relsec_shdr.sh_size);
    if (!rels) {
        fatal("could not allocate memory for rel.text section\n");
    }

    elf_section* relocated_section = find_section_by_index(relsec_shdr.sh_info, elf);

    for (Elf32_Word i = 0; i < rel_num; i++) {
        Elf64_Rela* rela64 = (Elf64_Rela*)(rela_section->s_data) + i;
        Elf32_Rel* rel32 = rels + i;
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

        adjust_addend(relocated_section, rel32->r_offset, rela64->r_addend);
    }

    free(rela_section->s_data);
    rela_section->s_data = rels;
    rela_section->s_header = relsec_shdr;
}

void create_reltext(elf_file* elf) {
    const char* name = ".rel.text";
    Elf32_Shdr reltext_shdr;
    reltext_shdr.sh_name = shstrtab_push(elf, name);
    reltext_shdr.sh_type = SHT_REL;
    reltext_shdr.sh_flags = SHF_INFO_LINK;
    reltext_shdr.sh_addr = 0;
    reltext_shdr.sh_offset = 0;
    reltext_shdr.sh_size = 0;
    reltext_shdr.sh_link = section_index(".symtab", elf);
    reltext_shdr.sh_info = section_index(".text", elf);
    reltext_shdr.sh_addralign = 8;
    reltext_shdr.sh_entsize = sizeof(Elf32_Rel);

    elf->e_header.e_shnum++;

    elf_section reltext = {
        .s_elf = elf,
        .s_header = reltext_shdr,
        .s_data = NULL,
    };
    list_add(elf->e_sections, &reltext);
}

void convert_sections(elf_file* elf) {
    elf_section* shstrtab = find_section(".shstrtab", elf);

    printf("\nconverting sections:\n");
    printf("> .symtab\n");

    convert_symtab(find_section(".symtab", elf));

    iterate_list(elf->e_sections, node) {
        elf_section* section = list_element(node);
        if (section->s_header.sh_type == SHT_RELA) {
            printf("> %s from RELA to REL\n", (char*)shstrtab->s_data + section->s_header.sh_name);
            convert_rela(section);
        }
    }

    elf_section* reltext = try_find_section(".rel.text", elf);
    if (!reltext) {
        printf("no .rel.text section found\n");
        printf("creating .rel.text section\n");
        create_reltext(elf);
    }
}

void check_header(Elf64_Ehdr header) {
    if (header.e_type != ET_REL) {
        fatal("not a relocatable file\n");
    }
    if (header.e_machine != EM_X86_64) {
        fatal("not a 64-bit x86 file\n");
    }
    if (header.e_shoff == 0) {
        fatal("section header offset not found\n");
    }

    if (header.e_entry != 0) {
        warn("entry point is not 0\n");
    }
    if (header.e_phoff != 0) {
        warn("program header offset found\n");
    }
    if (header.e_phnum != 0) {
        warn("program header number found\n");
    }
    if (header.e_phentsize != 0) {
        warn("program header entry size found\n");
    }
}
