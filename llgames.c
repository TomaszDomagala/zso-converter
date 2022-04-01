// nasm -f elf64 asmbits.asm
// gcc -no-pie -ggdb -Wall -O2 -o llgames llgames.c asmbits.o

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <sys/mman.h>

extern void call_64_from_64(void *buf);
extern void call_32_from_64(void *buf);
extern void call_32_from_32(void *buf);

int main(int argc, char **argv) {
    printf("Hello world!\n");

    char *buf = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    if (buf == MAP_FAILED) {
        perror("mmap");
        return -1;
    }

    printf("buf = %p\n", buf);

    strcpy(buf +   0x0, "Hello from 64-bit syscall from 64-bit code\n");
    strcpy(buf + 0x100, "Hello from 32-bit syscall from 64-bit code\n");
    // strcpy(buf + 0x200, "Hello from 64-bit syscall from 32-bit code\n");
    strcpy(buf + 0x300, "Hello from 32-bit syscall from 32-bit code\n");

    call_64_from_64(buf +   0x0);
    call_32_from_64(buf + 0x100);
    call_32_from_32(buf + 0x300);

    return 0;
}
