#include <stdio.h>

int main(void) {
    // Declare two variables for comparison
    int pokeballs = 10;
    int berries = 5;

    // Relational operator examples
    printf("Relational Operators in Pokémon Go Style:\n");

    // Equality (==)
    if (pokeballs == berries) {
        printf("Pokéballs and berries are equal in number!\n");
    } else {
        printf("Pokéballs and berries are not equal.\n");
    }

    // Inequality (!=)
    if (pokeballs != berries) {
        printf("You have a different number of Pokéballs and berries.\n");
    } else {
        printf("You have the same number of Pokéballs and berries.\n");
    }

    // Less than (<)
    if (pokeballs < berries) {
        printf("You have fewer Pokéballs than berries.\n");
    } else {
        printf("You have more or equal Pokéballs compared to berries.\n");
    }

    // Greater than (>)
    if (pokeballs > berries) {
        printf("You have more Pokéballs than berries.\n");
    } else {
        printf("You have fewer or equal Pokéballs compared to berries.\n");
    }

    // Less than or equal to (<=)
    if (pokeballs <= berries) {
        printf("You have fewer or equal Pokéballs compared to berries.\n");
    } else {
        printf("You have more Pokéballs than berries.\n");
    }

    // Greater than or equal to (>=)
    if (pokeballs >= berries) {
        printf("You have more or equal Pokéballs compared to berries.\n");
    } else {
        printf("You have fewer Pokéballs than berries.\n");
    }

    // Personal Message
    puts("Relational operators are fun! Ready to catch 'em all while mastering C programming?");

    return 0;
}
