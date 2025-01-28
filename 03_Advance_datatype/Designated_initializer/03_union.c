#include <stdio.h>

union Data {
    int i;
    float f;
    char c;
};

int main() {
    // Designated initializer for union
    union Data d = { .f = 3.14 };

    printf("Union data:\n");
    printf("f = %.2f\n", d.f);

    return 0;
}
