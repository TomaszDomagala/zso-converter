int r64(int, int *);

extern void print_ptr(void *ptr);
extern void print_str(char *str);

int r32(int depth, int *verify) {
    int test __attribute__((aligned(16)));
    print_str("previous pointer of r64: ");
    print_ptr(verify);
    if (((long)verify) & (0xf))
        return 1;

    /* just pass */
    return r64(depth, &test);
}
