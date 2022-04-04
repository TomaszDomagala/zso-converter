#include <elf.h>
#include <stdint.h>

#include "list.h"

typedef struct {
    Elf32_Shdr s_header;
    uint8_t* s_data;
} elf_section;

typedef struct {
    Elf32_Ehdr e_header;
    list_t* e_sections;
} elf_file;

elf_file* parse_elf(char* filename);

elf_section* get_section_by_name(elf_file* file, const char* name);

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