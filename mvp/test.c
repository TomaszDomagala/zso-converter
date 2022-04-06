#include <stdio.h>
#define MAXDEPTH 4
int r32(int, int *);

void print_str(char *str) {
	printf("%s", str);
}

void print_ptr(void *ptr) {
    printf("%p\n", ptr);
}

int r64(int depth, int *verify) {
    int test __attribute__((aligned(16)));
	print_str("previous pointer of r32: ");
	print_ptr(verify);
    // if (((long)verify) & 0xf)
    //     return 1;

    if (depth == MAXDEPTH) {
        return 0;
    } else
        return r32(depth + 1, &test);
}

int real_main() {
    if (r64(0, (int *)0x10) != 0)
        return -1;

    printf("OK\n");
    return 0;
}

int main() {
    return real_main();
}
