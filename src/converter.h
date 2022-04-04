#include <elf.h>
#include <stdio.h>

#include "list.h"

void convert_elf(Elf64_Ehdr ehdr, Elf64_Shdr *shdrs, FILE *elf64file, list_t *functions);
