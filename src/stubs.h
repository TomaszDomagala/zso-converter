#ifndef STUBS_H
#define STUBS_H

#include "elffile.h"
#include "list.h"

void build_stubs(elf_file *elf, list_t *functions);

#endif