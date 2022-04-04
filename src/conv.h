#ifndef CONV_H
#define CONV_H

#include <elf.h>
#include "elffile.h"

void check_header(Elf64_Ehdr header);

/**
 * @brief initial convert elf header from 64 to 32 
 * 
 * @param ehdr64 the 64 bit header to convert
 * @return Elf32_Ehdr the converted 32 bit header
 */
Elf32_Ehdr convert_elf_header(Elf64_Ehdr ehdr64);

/**
 * @brief initial convert elf section header from 64 to 32
 * 
 * @param shdr64 the 64 bit section header to convert
 * @return Elf32_Shdr the converted 32 bit section header
 */
Elf32_Shdr convert_section_header(Elf64_Shdr shdr64);

/**
 * @brief convert section data, convertion method depends on section type
 * 
 * @param section the section to convert
 */
void convert_section_data(elf_section* section);

#endif
