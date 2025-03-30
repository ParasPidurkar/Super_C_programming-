#include <stdio.h>

int main(void) {
    // Declare variables representing two Pokémon candies
    int pikachuCandy = 5;
    int eeveeCandy = 5;

    // Declare pointers to the candy variables
    int *pikachuPtr = &pikachuCandy;
    int *eeveePtr = &eeveeCandy;

    // Compare the memory addresses of the pointers
    if (pikachuPtr == eeveePtr) {
        printf("Pikachu and Eevee are sharing the same candy spot!\n");
    } else {
        printf("Pikachu and Eevee have their candies stored in different locations.\n");
    }

    // Compare the values pointed to by the pointers
    if (*pikachuPtr == *eeveePtr) {
        printf("Pikachu and Eevee have the same amount of candy: %d each!\n", *pikachuPtr);
    } else {
        printf("Pikachu and Eevee have different candy amounts.\n");
    }

    // Display a personal message
    puts("Hello! I am Paras, a Pokémon Trainer who loves playing Pokémon Go and coding!");

    return 0;
}
