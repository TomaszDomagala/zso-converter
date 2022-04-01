#include <stdio.h>

int fun_stub(int);

int main(){
    for (int i = 0; i < 10; i++) {
        printf("%d\n", fun_stub(i));
    }

    return 0;
}