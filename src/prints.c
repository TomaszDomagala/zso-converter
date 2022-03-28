#include "prints.h"
#include <stdio.h>

// typedef struct
// {
//   unsigned char	e_ident[EI_NIDENT];	/* Magic number and other info */
//   Elf64_Half	e_type;			/* Object file type */
//   Elf64_Half	e_machine;		/* Architecture */
//   Elf64_Word	e_version;		/* Object file version */
//   Elf64_Addr	e_entry;		/* Entry point virtual address */
//   Elf64_Off	e_phoff;		/* Program header table file offset */
//   Elf64_Off	e_shoff;		/* Section header table file offset */
//   Elf64_Word	e_flags;		/* Processor-specific flags */
//   Elf64_Half	e_ehsize;		/* ELF header size in bytes */
//   Elf64_Half	e_phentsize;		/* Program header table entry size */
//   Elf64_Half	e_phnum;		/* Program header table entry count */
//   Elf64_Half	e_shentsize;		/* Section header table entry size */
//   Elf64_Half	e_shnum;		/* Section header table entry count */
//   Elf64_Half	e_shstrndx;		/* Section header string table index */
// } Elf64_Ehdr;
void print_ehdr(Elf64_Ehdr *ehdr) {
    printf("e_ident[EI_OSABI]: %d\n", ehdr->e_ident[EI_OSABI]);
    printf("e_ident[EI_ABIVERSION]: %d\n", ehdr->e_ident[EI_ABIVERSION]);
    printf("e_type: %d\n", ehdr->e_type);
    printf("e_machine: %d\n", ehdr->e_machine);
    printf("e_version: %d\n", ehdr->e_version);
    printf("e_entry: %ld\n", ehdr->e_entry);
    printf("e_phoff: %ld\n", ehdr->e_phoff);
    printf("e_shoff: %ld\n", ehdr->e_shoff);
    printf("e_flags: %d\n", ehdr->e_flags);
    printf("e_ehsize: %d\n", ehdr->e_ehsize);
    printf("e_phentsize: %d\n", ehdr->e_phentsize);
    printf("e_phnum: %d\n", ehdr->e_phnum);
    printf("e_shentsize: %d\n", ehdr->e_shentsize);
    printf("e_shnum: %d\n", ehdr->e_shnum);
    printf("e_shstrndx: %d\n", ehdr->e_shstrndx);
}

// typedef struct {
//     uint32_t   sh_name;
//     uint32_t   sh_type;
//     uint64_t   sh_flags;
//     Elf64_Addr sh_addr;
//     Elf64_Off  sh_offset;
//     uint64_t   sh_size;
//     uint32_t   sh_link;
//     uint32_t   sh_info;
//     uint64_t   sh_addralign;
//     uint64_t   sh_entsize;
// } Elf64_Shdr;
void print_shdr(Elf64_Shdr *shdr) {
    printf("sh_name: %d\n", shdr->sh_name);
    printf("sh_type: %d\n", shdr->sh_type);
    printf("sh_flags: %ld\n", shdr->sh_flags);
    printf("sh_addr: %ld\n", shdr->sh_addr);
    printf("sh_offset: %ld\n", shdr->sh_offset);
    printf("sh_size: %ld\n", shdr->sh_size);
    printf("sh_link: %d\n", shdr->sh_link);
    printf("sh_info: %d\n", shdr->sh_info);
    printf("sh_addralign: %ld\n", shdr->sh_addralign);
    printf("sh_entsize: %ld\n", shdr->sh_entsize);
}

// typedef struct
// {
//   Elf64_Word	st_name;		/* Symbol name (string tbl index) */
//   unsigned char	st_info;		/* Symbol type and binding */
//   unsigned char st_other;		/* Symbol visibility */
//   Elf64_Section	st_shndx;		/* Section index */
//   Elf64_Addr	st_value;		/* Symbol value */
//   Elf64_Xword	st_size;		/* Symbol size */
// } Elf64_Sym;
void print_sym(Elf64_Sym *sym) {
    printf("st_name: %d\n", sym->st_name);
    printf("st_info: %d\n", sym->st_info);
    printf("st_other: %d\n", sym->st_other);
    printf("st_shndx: %d\n", sym->st_shndx);
    printf("st_value: %ld\n", sym->st_value);
    printf("st_size: %ld\n", sym->st_size);
}
