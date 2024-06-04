#include <stdio.h>

int main() {
    // Variable declarations with different names
    int age = 25;
    float weight_kg = 65.5;
    double height_m = 1.75;
    char initial = 'J';
    char name[] = "John";

    // Print the variables
    printf("Age: %d\n", age);
    printf("Weight (kg): %.1f\n", weight_kg);
    printf("Height (m): %.2lf\n", height_m);
    printf("Initial: %c\n", initial);
    printf("Name: %s\n", name);

    return 0;
}
