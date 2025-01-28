#include <stdio.h>

int main() {
    // Designated initializer for array
    int arr[10] = { [0] = 1, [3] = 5, [5] = 10 };

    printf("Array elements:\n");
    for (int i = 0; i < 10; i++) {
        printf("arr[%d] = %d\n", i, arr[i]);
    }

    return 0;
}
