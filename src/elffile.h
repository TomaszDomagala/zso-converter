#ifndef ELFFILE_H
#define ELFFILE_H

#include <elf.h>
#include <stdint.h>

#include "list.h"

typedef struct elf_section {
    struct elf_file* s_elf;
    Elf32_Shdr s_header;
    uint8_t* s_data;
} elf_section;

typedef struct elf_file {
    Elf32_Ehdr e_header;
    list_t* e_sections;
} elf_file;

/**
 * @brief reads ELF64 from file and performs initial conversions to ELF32
 *
 * @param filename the elf64 file to read
 * @return elf_file* the initialy converted elf32 file
 */
elf_file* read_elf(char* filename);

elf_section* find_section(const char* name, elf_file* file);

char* section_name(elf_section* section);

/**
 * @brief adds a symbol to the .symtab section
 *
 * @param file the ELF file to add the symbol to
 * @param sym the symbol to add
 * @return Elf32_Word the index of the symbol in the symbol table
 */
Elf32_Word symtab_push(elf_file* file, Elf32_Sym* sym);

/**
 * @brief adds a string to the .strtab table
 *
 * @param file the ELF file to add the string to
 * @param str the string to add
 * @return Elf32_Word the index of the string in the string table
 */
Elf32_Word strtab_push(elf_file* file, char* str);

/**
 * @brief adds a rel to the .rel.text section
 *
 * @param file the ELF file to add the rel to
 * @param rel the rel to add
 * @return Elf32_Word the index of the rel in the rel section
 */
Elf32_Word rel_push(elf_file* file, Elf32_Rel* rel);

/**
 * @brief adds bytes to the end of the .text section
 *
 * @param file the ELF file to add the bytes to
 * @param bytes the bytes to add
 * @param size the number of bytes to add
 *
 * @return size_t the number of bytes added
 */
size_t text_push(elf_file* file, char* bytes, size_t size);

#endif
