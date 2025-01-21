#include <stdio.h>

int initializer() {
    return 50;
}

int fun() {
    static int count = initializer(); // Static variable initialized with a function
    int localvar = 0;

    printf("automatic=%d, static=%d\n", localvar, count);

    count++;
    localvar++;
}

int main() {
    fun();
    fun(); // Call multiple times to observe behavior of static variable
    return 0;
}
