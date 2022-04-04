#include "elffile.h"

#include <stdio.h>

#include "conv.h"
#include "fatal.h"

struct elf_section {
    struct elf_file* elf;
    Elf32_Shdr s_header;
    uint8_t* s_data;
};

struct elf_file {
    Elf32_Ehdr e_header;
    list_t* e_sections;
};

elf_file* read_elf(char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        sysfatalf("fopen", "could not open %s\n", filename);
    }

    Elf64_Ehdr temp_header64;
    fread(&temp_header64, sizeof(temp_header64), 1, file);
    if (ferror(file)) {
        sysfatalf("fread", "could not read %s\n", filename);
    }
    check_header(temp_header64);

    Elf32_Ehdr header32 = convert_elf_header(temp_header64);
    elf_file* elf32 = malloc(sizeof(elf_file));
    if (!elf32) {
        sysfatal("malloc", "could not allocate memory for elf_file\n");
    }
    elf32->e_header = header32;

    list_t* sections = list_create(sizeof(elf_section));

    if (-1 == fseek(file, header32.e_shoff, SEEK_SET)) {
        sysfatalf("fseek", "could not seek to section header table in %s\n", filename);
    }


    // section creation and initial header conversion
    for (int i = 0; i < header32.e_shnum; i++) {
        Elf64_Shdr temp_shdr64;
        fread(&temp_shdr64, sizeof(temp_shdr64), 1, file);
        if (ferror(file)) {
            sysfatalf("fread", "could not read %s\n", filename);
        }
        Elf32_Shdr section_header = convert_section_header(temp_shdr64);
        elf_section section = {
            .s_elf = elf32,
            .s_header = section_header,
            .s_data = NULL,
        };
        list_append(sections, &section);
    }

    // section data reading
    iterate_list(sections, node) {
        elf_section* section = list_element(node);
        if (-1 == fseek(file, section->s_header.sh_offset, SEEK_SET)) {
            sysfatalf("fseek", "could not seek to section data in %s\n", filename);
        }
        section->s_data = malloc(section->s_header.sh_size);
        if (section->s_data == NULL) {
            sysfatalf("malloc", "could not allocate memory for section data in %s\n", filename);
        }
        size_t bytes_read = fread(section->s_data, section->s_header.sh_size, 1, file);
        if (ferror(file)) {
            sysfatalf("fread", "could not read section data in %s\n", filename);
        }
        if (bytes_read != 1) {
            fatalf("could not read section data in %s\n", filename);
        }
    }
    fclose(file);

    // section data conversion
    iterate_list(sections, node) {
        elf_section* section = list_element(node);
        convert_section_data(section);
    }


    return elf32;
}
