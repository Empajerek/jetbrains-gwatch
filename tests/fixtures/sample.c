#include <stdio.h>
#include <unistd.h>

volatile int global_variable = 0;

void sample_function(int n) {
    for (int i = 1; i <= n; i++) {
        global_variable += i;
    }

}

int main() {
    sample_function(100);
    printf("%p\n", (void*) &global_variable);
    printf("Value of global variable: %d, and size of long: %lu\n", global_variable, sizeof(long));
    sleep(1);
    return 0;
}
