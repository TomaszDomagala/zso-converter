#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include "elf.h"
#include "stdio.h"
#include "string.h"

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

// typedef struct
// {
//   Elf64_Word	st_name;		/* Symbol name (string tbl index) */
//   unsigned char	st_info;		/* Symbol type and binding */
//   unsigned char st_other;		/* Symbol visibility */
//   Elf64_Section	st_shndx;		/* Section index */
//   Elf64_Addr	st_value;		/* Symbol value */
//   Elf64_Xword	st_size;		/* Symbol size */
// } Elf64_Sym;

void sysfatal(const char *func, const char *format, ...) __attribute__ ((noreturn));
void fatal(const char *format, ...) __attribute__ ((noreturn));


void sysfatal(const char *func, const char *format, ...) {
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
    fprintf(stderr, "%s: %s\n", func, strerror(errno));
    fprintf(stderr, "errno: %d\n", errno);
    exit(errno);
}

void fatal(const char *format, ...) {
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
    exit(1);
}

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

void print_sym(Elf64_Sym *sym) {
    printf("st_name: %d\n", sym->st_name);
    printf("st_info: %d\n", sym->st_info);
    printf("st_other: %d\n", sym->st_other);
    printf("st_shndx: %d\n", sym->st_shndx);
    printf("st_value: %ld\n", sym->st_value);
    printf("st_size: %ld\n", sym->st_size);
}


int main(int argc, char *argv[]) {
    if (argc != 4) {
        fatal("Usage: %s <ET_REL file> <functions file> <output ET_REL file>\n", argv[0]);
    }
    FILE *in = fopen(argv[1], "rb");

    if (!in) {
        sysfatal("fopen", "Could not open %s\n", argv[1]);
    }

    Elf64_Ehdr ehdr;
    if (-1 == fread(&ehdr, sizeof(ehdr), 1, in)) {
        sysfatal("fread", "Could not read %s\n", argv[1]);
    }
    if (ehdr.e_shoff == 0) {
        fatal("No section header table found\n");
    }
    print_ehdr(&ehdr);
    // TODO: handle e_shnum and e_shstrndx in initial entry

    size_t shdrs_size = ehdr.e_shentsize * ehdr.e_shnum;
    Elf64_Shdr *shdrs = malloc(shdrs_size);

    if (shdrs == NULL) {
        sysfatal("malloc", "Could not allocate memory for section header table");
    }
    if (-1 == fseek(in, ehdr.e_shoff, SEEK_SET)) {
        sysfatal("fseek", "Could not set position to section header table");
    }

    size_t sections_read = 0;
    while (sections_read < ehdr.e_shnum) {
        size_t read = fread(shdrs + sections_read, ehdr.e_shentsize, ehdr.e_shnum - sections_read, in);
        if (read <= 0 && ferror(in) != 0) {
            sysfatal("fread", "Could not read section header table");
        }
        sections_read += read;
    }

    Elf64_Shdr shstrtab_section = shdrs[ehdr.e_shstrndx];
    char *shstrtab = malloc(shstrtab_section.sh_size);
    if (shstrtab == NULL) {
        sysfatal("malloc", "Could not allocate memory for string table");
    }
    if (-1 == fseek(in, shstrtab_section.sh_offset, SEEK_SET)) {
        sysfatal("fseek", "Could not set position to string table");
    }
    if (-1 == fread(shstrtab, shstrtab_section.sh_size, 1, in)) {
        sysfatal("fread", "Could not read string table");
    }



    for (int i = 0; i < ehdr.e_shnum; i++) {
        printf("\nSection %d: %s\n", i, shstrtab + shdrs[i].sh_name);
        print_shdr(shdrs + i);
    }

    Elf64_Shdr symtab_section;
    
    size_t symtab_found = 0;
    for (int i = 0; i < ehdr.e_shnum; i++) {
        if (shdrs[i].sh_type == SHT_SYMTAB) {
            if (symtab_found) {
                fatal("Multiple symbol tables found\n");
            }

            symtab_section = shdrs[i];
            symtab_found += 1;
        }
    }
    if (!symtab_found) {
        fatal("No symbol table found\n");
    }
    
    Elf64_Sym *symtab = malloc(symtab_section.sh_size);
    if (symtab == NULL) {
        sysfatal("malloc", "Could not allocate memory for symbol table");
    }
    if (-1 == fseek(in, symtab_section.sh_offset, SEEK_SET)) {
        sysfatal("fseek", "Could not set position to symbol table");
    }
    if (-1 == fread(symtab, symtab_section.sh_size, 1, in)) {
        sysfatal("fread", "Could not read symbol table");
    }

    Elf64_Shdr strtab_section = shdrs[symtab_section.sh_link];
    char *strtab = malloc(strtab_section.sh_size);
    if (strtab == NULL) {
        sysfatal("malloc", "Could not allocate memory for string table");
    }
    if (-1 == fseek(in, strtab_section.sh_offset, SEEK_SET)) {
        sysfatal("fseek", "Could not set position to string table");
    }
    if (-1 == fread(strtab, strtab_section.sh_size, 1, in)) {
        sysfatal("fread", "Could not read string table");
    }

    for (int i = 0; i < symtab_section.sh_size / symtab_section.sh_entsize; i++) {
        printf("\nSymbol %d: %s\n", i, strtab + symtab[i].st_name);
        print_sym(symtab + i);
    }


    return 0;
}