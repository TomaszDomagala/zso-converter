#include <sys/mman.h>
#include <stdio.h>

extern int foo(int x);

int real_main() {
	return foo(1);
}

int main() {
	int res = real_main();
	printf("res: %d\n", res);
	return 0;
}
