#include <stdio.h>

// Declare the extern variable
extern int chakra_level;

// Declare the function from naruto.c
extern void train_naruto();

void kakashi_test() {
    printf("Kakashi checks Naruto's initial chakra level: %d\n", chakra_level);
    train_naruto(); // Call Naruto's training function
    printf("Kakashi rechecks Naruto's chakra level: %d\n", chakra_level);
}
