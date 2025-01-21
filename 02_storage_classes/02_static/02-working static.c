#include <stdio.h>

int initializer() {
    return 50;
}

static int count = 0; // Static variable defined globally
static int initialized = 0;

void fun() {
    if (!initialized) {
        count = initializer(); // Initialize once using the function
        initialized = 1;
    }

    int localvar = 0;

    printf("automatic=%d, static=%d\n", localvar, count);

    count++;
    localvar++;
}

int main() {
    fun();
    fun(); // Call multiple times to see the behavior of static variable
    return 0;
}
