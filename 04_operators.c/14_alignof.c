#include <stdio.h>
#include <stdalign.h>

int main() {
    printf("Alignment of char: %zu\n", alignof(char));
    printf("Alignment of int: %zu\n", alignof(int));
    printf("Alignment of double: %zu\n", alignof(double));
    return 0;
}