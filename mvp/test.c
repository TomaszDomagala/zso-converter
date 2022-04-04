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
extern long long bar(int x, long long y);

// int doo(int x){
// 	int y = x + 1;
// 	return y;
// }

long long real_main(){
	int x = 20;
	long long y = 30;
	long long z = bar(x, y);
	return z;
}

int main() {
	int res = real_main();
	printf("res: %ld\n", res);
	return 0;
}
