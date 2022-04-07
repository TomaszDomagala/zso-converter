# zso-converter

## [PL]

Rozwiązanie jest napisane w języku C. Pliki źródłowe znajdują się w directory `/src`.

Kompilacja za pomocą polecenia `make` w głównym directory projektu.
Make stworzy `converter`, którego można użyć zgodnie z poleceniem `./converter <plik ET_REL> <plik z listą funkcji> <docelowy plik ET_REL>`.

Opis plików:

- conv.h - dostarcza funkcje konwertujące Elf64_Ehdr na Elf32_Ehdr, Elf64_Shdr na Elf32_Shdr, Elf64_Rela na Elf32_Rel i Elf64_Sym na Elf32_Sym.
- elffile.h - dostarcza funkcję `read_elf` czytającą plik ELF, oraz kilka utility funkcji ułatwiających znajdowanie sekcji itp.
- stubs.h - dostarcza funkcję `build_stubs`, która generuje stuby dla funkcji globalnych i zewnętrznych zgonie z treścią zadania
- list.h - doubly linked list
- fatal.h - funkcje obsługujące errory
