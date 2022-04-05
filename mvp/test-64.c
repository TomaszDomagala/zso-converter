// extern int doo(int x);
#include <stdio.h>

extern void doo(char *str);

int foo(int x) {
	return x;
}

int bar(int a, int b) {
	doo("hello bcz");

	return foo(a + b);
}

