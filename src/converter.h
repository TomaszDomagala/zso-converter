#include <elf.h>
#include <stdio.h>

void convert_elf(Elf64_Ehdr ehdr, Elf64_Shdr *shdrs, FILE *elf64file);
