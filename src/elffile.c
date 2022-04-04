#include "elffile.h"

#include <stdio.h>
#include <string.h>

#include "conv.h"
#include "fatal.h"

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
    elf32->e_sections = sections;

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
        list_add(sections, &section);
    }

    // section data reading
    iterate_list(sections, node) {
        elf_section* section = list_element(node);
        if (section->s_header.sh_type == SHT_NOBITS) {
            continue;
        }
        if (section->s_header.sh_size == 0) {
            continue;
        }

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

    printf("file %s contains %ld sections\n", filename, list_size(sections));

    return elf32;
}

elf_section* get_shstrtab(elf_file* elf) {
    int index = 0;
    iterate_list(elf->e_sections, node) {
        if (index == elf->e_header.e_shstrndx) {
            return list_element(node);
        }
        index++;
    }
    fatalf("could not find shstrtab at e_shstrndx: %d\n", elf->e_header.e_shstrndx);
}

char* section_name(elf_section* section) {
    elf_section* shstrtab = get_shstrtab(section->s_elf);
    return (char*)shstrtab->s_data + section->s_header.sh_name;
}

elf_section* find_section(const char* name, elf_file* file) {
    iterate_list(file->e_sections, node) {
        elf_section* section = list_element(node);
        if (strcmp(section_name(section), name) == 0) {
            return section;
        }
    }
    fatalf("could not find section %s\n", name);
}

void write_section_data(FILE* file, elf_section* section) {
    if (section->s_header.sh_type == SHT_NOBITS) {
        return;
    }
    if (section->s_header.sh_size == 0) {
        return;
    }
    long current_pos = ftell(file);
    // not sure if this is necessary, but it doesn't hurt
    if (section->s_header.sh_addralign > 1 && current_pos % section->s_header.sh_addralign != 0) {
        long padding = section->s_header.sh_addralign - (current_pos % section->s_header.sh_addralign);
        void* padding_data = calloc(padding, 1);
        fwrite(padding_data, padding, 1, file);
        if (ferror(file)) {
            sysfatal("fwrite", "could not write padding data to file\n");
        }
        free(padding_data);
        current_pos += padding;
    }

    section->s_header.sh_offset = current_pos;
    fwrite(section->s_data, section->s_header.sh_size, 1, file);
    if (ferror(file)) {
        sysfatal("fwrite", "could not write section data to file\n");
    }
}

void write_elf(elf_file* elf, char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        sysfatalf("fopen", "could not open %s\n", filename);
    }

    // skip file header for now, as we need to know value of e_shoff
    if (-1 == fseek(file, sizeof(elf->e_header), SEEK_SET)) {
        sysfatalf("fseek", "could not seek to start of data segment %s\n", filename);
    }

    // write sections data
    iterate_list(elf->e_sections, node) {
        elf_section* section = list_element(node);
        write_section_data(file, section);
    }
    elf->e_header.e_shoff = ftell(file);

    // write sections headers
    iterate_list(elf->e_sections, node) {
        elf_section* section = list_element(node);
        fwrite(&section->s_header, sizeof(section->s_header), 1, file);
        if (ferror(file)) {
            sysfatalf("fwrite", "could not write %s\n", filename);
        }
    }

    if (-1 == fseek(file, 0, SEEK_SET)) {
        sysfatalf("fseek", "could not seek to start of file %s\n", filename);
    }
    fwrite(&elf->e_header, sizeof(elf->e_header), 1, file);
    if (ferror(file)) {
        sysfatalf("fwrite", "could not write %s\n", filename);
    }

    fclose(file);
}