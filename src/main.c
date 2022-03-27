#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>

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

// Usage: ./converter <ET_REL file> <functions file> <output ET_REL file>
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

    Elf64_Shdr *shdrs = malloc(ehdr.e_shentsize * ehdr.e_shnum);

    if (shdrs == NULL) {
        sysfatal("malloc", "Could not allocate memory for section header table");
    }
    if (-1 == fseek(in, ehdr.e_shoff, SEEK_SET)) {
        sysfatal("fseek", "Could not set position to section header table");
    }

    return 0;
}