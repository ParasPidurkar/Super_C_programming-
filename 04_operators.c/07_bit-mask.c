#include <stdio.h>
#include <limits.h>

void bit_pattern(int u) {
    int i, x, word;
    unsigned mask = 1;

    word = CHAR_BIT * sizeof(int); // Calculate number of bits in int
    mask = mask << (word - 1);    // Shift mask to the leftmost bit

    for (i = 1; i <= word; i++) {
        x = (u & mask) ? 1 : 0;   // Check if the bit is 1 or 0
        printf("%d", x);          // Print the bit
        mask >>= 1;               // Shift mask to the right
    }
    printf("\n"); // End with a newline for readability
}

int main() {
    int number = 5;
    printf("Binary representation of %d:\n", number);
    bit_pattern(number);

    return 0;
}
