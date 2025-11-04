#include <stdio.h>
#include <string.h>

volatile int z = 0; // disable optimization

int main() {
    char command[10];
    int value;

    while (1) {
        if (scanf("%9s", command) != 1) break;

        if (strcmp(command, "write") == 0) {
            if (scanf("%d", &value) != 1) break;
            z = value;
        }
        else if (strcmp(command, "add") == 0) {
            if (scanf("%d", &value) != 1) break;
            z += value;
        }
        else if (strcmp(command, "read") == 0) {
            volatile int y = z; // ensure that compiler doesn't change it
            (void) y;
        }
        else if (strcmp(command, "exit") == 0) {
            break;
        }
    }

    return 0;
}
