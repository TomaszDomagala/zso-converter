// extern int doo(int x);
#include <stdio.h>

int foo(int x) {
	int z = x * 2;
	return z;
}

int bar(unsigned int a, int b, int c) {
	int k = foo(a+b+c);
	return k;
}

long long id(long long x) {
	return x;
}
long long add(long long x, long long y) {
	return x + y;
}
void mod(char *msg){
	msg[0] = 'x';
}

