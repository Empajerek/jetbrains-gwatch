#include <stdio.h>
#include <string.h>

volatile long z = 0;

int main() {
    for (int i = 0; i < 1e4; i++) {
        printf("print something so its long\n");
        z += i;
        printf("Now the value of z is %ld\n", z);
    }

    return 0;
}
