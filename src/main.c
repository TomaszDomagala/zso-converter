#include <elf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "elffile.h"
#include "fatal.h"
#include "funcfile.h"
#include "list.h"
#include "prints.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fatalf("Usage: %s <ET_REL file> <functions file> <output ET_REL file>\n", argv[0]);
    }

    elf_file* elf = read_elf(argv[1]);

    write_elf(elf, argv[3]);

    return 0;
}