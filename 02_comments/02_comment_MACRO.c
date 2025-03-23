#include <stdio.h>

/* Define the macro to control code inclusion */
#define TEAM_INSTINCT  // Change this to control which block of code is included

int main() {
    printf("Welcome to Pokémon GO!\n");

#ifdef TEAM_INSTINCT
    /* Code for Team Instinct trainers */
    printf("Zapdos has powered up your Pokémon!\n");
    printf("You're full of instinct and ready to battle!\n");
#elif defined(TEAM_MYSTIC)
    /* Code for Team Mystic trainers */
    printf("Articuno is guiding you with wisdom!\n");
    printf("Strategy and knowledge are your strengths.\n");
#elif defined(TEAM_VALOR)
    /* Code for Team Valor trainers */
    printf("Moltres inspires your fiery determination!\n");
    printf("Your passion burns brightly in every battle.\n");
#else
    /* Code for trainers with no team */
    printf("Choose a team to start your Pokémon journey!\n");
#endif

    return 0;
}
