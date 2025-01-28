#include <stdio.h>
#include <stdlib.h> // For malloc and free

// Define a structure with a flexible array member
struct s {
    int arraySize; // Fixed member
    int array[];   // Flexible array member (empty brackets)
};

int main() {
    int desiredSize = 5; // Desired size of the flexible array
    struct s *ptr;

    // Allocate memory for the structure and flexible array
    ptr = malloc(sizeof(struct s) + desiredSize * sizeof(int));
    if (ptr == NULL) {
        printf("Memory allocation failed!\n");
        return 1; // Exit on memory allocation failure
    }

    // Set the size of the array
    ptr->arraySize = desiredSize;

    // Initialize the flexible array with values
    for (int i = 0; i < desiredSize; i++) {
        ptr->array[i] = i + 1; // Assign values 1, 2, 3, ..., desiredSize
    }

    // Print the array values
    printf("Flexible array contents:\n");
    for (int i = 0; i < desiredSize; i++) {
        printf("%d ", ptr->array[i]);
    }
    printf("\n");

    // Print the size of the structure (ignoring the flexible array)
    printf("Size of struct (excluding flexible array): %lu bytes\n", sizeof(struct s));

    // Print total allocated memory
    printf("Total allocated memory: %lu bytes\n", sizeof(struct s) + desiredSize * sizeof(int));

    // Free the allocated memory
    free(ptr);

    return 0;
}

