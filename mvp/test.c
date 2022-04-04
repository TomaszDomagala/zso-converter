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
extern int bar(int x);

int real_main(){
	int x = 1000;
	x = bar(x);
	return x;
}

int main() {
	int res = real_main();
	printf("res: %d\n", res);
	return 0;
}
