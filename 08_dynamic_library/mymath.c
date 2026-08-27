#include "mymath.h"

int add(int a, int b) {
    return a + b;
}

int sub(int a, int b) {
    return a - b;
}

int mul(int a, int b) {
    return a * b;
}

int div_int(int a, int b) {
    return b == 0 ? 0 : a / b;
}
