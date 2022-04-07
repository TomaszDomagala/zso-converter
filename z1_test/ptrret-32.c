#include <stdio.h>
#include <string.h>

extern char* hello_return();

int real_main() {
    char* hello = hello_return();
    if (strcmp(hello, "hello world") != 0) {
        return -1;
    }
    printf("OK\n");

    return 0;
}