#include<stdio.h>
int *fn(void) {
        int pokemonLocation = 101;    // A unique ID representing a Pokémon's location.
        return &pokemonLocation; // Return the address of the Pokémon location.
    }

int catchPokemon(void) {
    return 25; // Returns Pokémon ID (e.g., 25 for Pikachu).
}


int main(){

    // int *location = fn();        // Get the location pointer from the function.
    // printf("Pokémon location: %d\n", *location); // Access the location value.




int (*fp)(void);

fp= catchPokemon; // `fp` points to the `catchPokemon` function.

int pokemonID = fp();          // Call the function through the pointer.
printf("Caught Pokémon ID: %d\n", pokemonID); // Prints the Pokémon's ID.

    return 0;
}