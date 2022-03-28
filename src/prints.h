#include <elf.h>

void print_ehdr(Elf64_Ehdr *ehdr);
void print_shdr(Elf64_Shdr *shdr);
void print_sym(Elf64_Sym *sym);
