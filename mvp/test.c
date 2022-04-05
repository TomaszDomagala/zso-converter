#include <sys/mman.h>
#include <stdio.h>

// extern int foo(int x);

// extern int bar(int x);

// int doo(int x){
// 	return x + 1;
// }

// int real_main() {
// 	int x = 1;
// 	x = foo(x);
// 	x = bar(x);
	
// 	return x;
// }
extern int bar(unsigned int a, int b, int c);
extern long long id(long long x);
extern long long add(long long x, long long y);
extern void mod(char *msg);

// int doo(int x){
// 	int y = x + 1;
// 	return y;
// }



long long real_main(){
	char hello[100];
	hello[0] = 'h';
	hello[1] = 'e';
	hello[2] = 'l';
	hello[3] = 'l';
	hello[4] = 'o';
	hello[5] = '\0';

	// 4,294,967,295
	long long x = add(4294967295ll, 429496720905ll);
	mod(hello);
	printf("%s\n", hello);
	return x;
}

int main() {
	long long res = real_main();
	printf("res: %lld\n", res);
	return 0;
}
