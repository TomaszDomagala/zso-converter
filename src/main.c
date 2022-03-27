#include "stdio.h"
#include "elf.h"
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


// Usage: ./converter <ET_REL file> <functions file> <output ET_REL file>
int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <ET_REL file> <functions file> <output ET_REL file>\n", argv[0]);
        return 1;
    }
    FILE *in = fopen(argv[1], "rb");
    if (!in) {
        printf("Could not open %s\n", argv[1]);
        return 1;
    }

    Elf64_Ehdr ehdr;
    fread(&ehdr, sizeof(ehdr), 1, in);
    fclose(in);

    printf("e_ident[EI_OSABI]: %d\n", ehdr.e_ident[EI_OSABI]);
    printf("e_ident[EI_ABIVERSION]: %d\n", ehdr.e_ident[EI_ABIVERSION]);
    printf("e_type: %d\n", ehdr.e_type);
    printf("e_machine: %d\n", ehdr.e_machine);
    printf("e_version: %d\n", ehdr.e_version);
    printf("e_entry: %ld\n", ehdr.e_entry);
    printf("e_phoff: %ld\n", ehdr.e_phoff);
    printf("e_shoff: %ld\n", ehdr.e_shoff);
    printf("e_flags: %d\n", ehdr.e_flags);
    printf("e_ehsize: %d\n", ehdr.e_ehsize);
    printf("e_phentsize: %d\n", ehdr.e_phentsize);
    printf("e_phnum: %d\n", ehdr.e_phnum);
    printf("e_shentsize: %d\n", ehdr.e_shentsize);
    printf("e_shnum: %d\n", ehdr.e_shnum);
    printf("e_shstrndx: %d\n", ehdr.e_shstrndx);

    return 0;
}