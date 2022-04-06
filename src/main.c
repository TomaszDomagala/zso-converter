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
#include "stubs.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fatalf("Usage: %s <ET_REL file> <functions file> <output ET_REL file>\n", argv[0]);
    }

    printf("\n");
    printf("elf file: %s\n", argv[1]);
    printf("functions file: %s\n", argv[2]);
    printf("output elf file: %s\n", argv[3]);

    elf_file* elf = read_elf(argv[1]);

    list_t* functions = read_funcs(argv[2]);

    elf_section* eh_frame = try_find_section(".eh_frame", elf);
    if (eh_frame != NULL) {
        memset(&eh_frame->s_header, 0, sizeof(eh_frame->s_header));
    }
    elf_section* rela_eh_frame = try_find_section(".rela.eh_frame", elf);
    if (rela_eh_frame != NULL) {
        memset(&rela_eh_frame->s_header, 0, sizeof(rela_eh_frame->s_header));
    }

    build_stubs(elf, functions);

    write_elf(elf, argv[3]);

    return 0;
}