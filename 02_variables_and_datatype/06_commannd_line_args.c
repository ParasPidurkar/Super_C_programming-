#include <stdio.h>

int main(int argc, char *argv[]) {
    // argc is the count of arguments passed to the program
    // argv is an array of strings containing the arguments

    // Print the count of command-line arguments
    printf("Number of command-line arguments: %d\n", argc);

    // Print each command-line argument
    printf("Command-line arguments:\n");
    for (int i = 0; i < argc; i++) {
        printf("Argument %d: %s\n", i, argv[i]);
    }

    return 0;
}
