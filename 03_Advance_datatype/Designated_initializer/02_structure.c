#include <stdio.h>

struct Point {
    int x;
    int y;
    int z;
};

int main() {
    // Designated initializer for struct
    struct Point p = { .x = 10, .z = 30 };

    printf("Point coordinates:\n");
    printf("x = %d, y = %d, z = %d\n", p.x, p.y, p.z);

    return 0;
}
