#include "stdio.h"
#include "elf.h"

// Usage: ./converter <ET_REL file> <functions file> <output ET_REL file>
int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <ET_REL file> <functions file> <output ET_REL file>\n", argv[0]);
        return 1;
    }
    FILE *in = fopen(argv[1], "rb");
    if (!in) {
        printf("Could not open %s\n", argv[1]);
        return 1;
    }

    return 0;
}