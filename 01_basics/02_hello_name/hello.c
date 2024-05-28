#include <stdio.h>
#include <stdlib.h>

int main() {
    // Allocate memory dynamically
    char *name = malloc(100 * sizeof(char));
    if (name == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    printf("Enter your name: ");
    scanf("%99s", name); // Use %99s to avoid buffer overflow
    printf("Hello, %s\n", name);

    // Free the allocated memory
    free(name);
    return 0;
}
