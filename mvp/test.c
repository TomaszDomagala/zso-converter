#include <sys/mman.h>
#include <stdio.h>


extern int bar(int a, int b);

void doo(char* str){
	printf("%s\n", str);
}


long long real_main(){
	int a = 1;
	int b = 2;

	return bar(a, b);
}

int main() {
	long long res = real_main();
	printf("res: %lld\n", res);
	return 0;
}
