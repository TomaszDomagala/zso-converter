#include <stdio.h>

long long foo(long long x) {
    return x + 1;
}

int main(){
    long long x = 2;
    x = foo(x);
    printf("x: %lld\n", x);

    return 0;
}