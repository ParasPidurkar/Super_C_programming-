#include <stdio.h>

int main() {
    // Declare variables
    int num1 = 20, num2 = 6;
    int sum, difference, product, quotient, remainder;

    // Arithmetic Operations
    sum = num1 + num2;          // Addition
    difference = num1 - num2;   // Subtraction
    product = num1 * num2;      // Multiplication
    quotient = num1 / num2;     // Division
    remainder = num1 % num2;    // Modulus (remainder)

    // Print Results
    printf("Arithmetic Operations:\n");
    printf("%d + %d = %d\n", num1, num2, sum);
    printf("%d - %d = %d\n", num1, num2, difference);
    printf("%d * %d = %d\n", num1, num2, product);
    printf("%d / %d = %d\n", num1, num2, quotient);
    printf("%d %% %d = %d\n", num1, num2, remainder);

    // Personal Message
    puts("Arithmetic operators are fundamental for any programmer. Keep practicing!");

    return 0;
}
