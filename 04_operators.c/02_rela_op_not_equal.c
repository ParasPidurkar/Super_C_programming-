#include <stdio.h>

int main(void) {
    // Pikachu's and Charmander's Pokéballs have candy stored inside
    int pikachuCandy = 5;
    int charmanderCandy = 5;

    // Declare pointers pointing to the candy locations
    int *pikachuPtr = &pikachuCandy;
    int *charmanderPtr = &charmanderCandy;

    // Compare the memory addresses of the pointers
    if (pikachuPtr != charmanderPtr) {
        printf("Pikachu and Charmander have their candies stored at different locations.\n");
    } else {
        printf("Pikachu and Charmander are sharing the same candies .\n");
    }

    // Compare the values stored at the locations the pointers point to
    if (*pikachuPtr != *charmanderPtr) {
        printf("Pikachu and Charmander have different amounts of candy.\n");
    } else {
        printf("Pikachu and Charmander have the same amount of candy: %d each!\n", *pikachuPtr);
    }

    return 0;
}
