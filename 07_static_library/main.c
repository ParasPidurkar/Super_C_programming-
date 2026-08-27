#include <stdio.h>
#include "mymath.h"

int main() {
    int a = 10, b = 3;

    printf("add(%d, %d) = %d\n", a, b, add(a, b));
    printf("sub(%d, %d) = %d\n", a, b, sub(a, b));
    printf("mul(%d, %d) = %d\n", a, b, mul(a, b));
    printf("div_int(%d, %d) = %d\n", a, b, div_int(a, b));

    return 0;
}
