#include <stdio.h>

// Valorant-style: The ultimate format specifiers showcase
int main(void)
{
    // Integer stats: kills, assists, deaths
    int kills = 25, assists = 15, deaths = 10;
    unsigned int headshots = 50; // unsigned value for headshots

    // Floating-point stats: accuracy, KD ratio
    float accuracy = 87.456f;      // float value
    double kd_ratio = (double)kills / deaths; // calculated double value

    // Character and string: player rank and tag
    char rank = 'A';               // single character for rank
    char player_tag[] = "PhantomAce"; // string for the player tag

    // Pointer: Memory location of kills variable
    void *kills_ptr = (void *)&kills;

    // Print formatted output with Valorant-style flair
    printf("\033[1;31m// VALORANT GAMING STATS: FORMAT SPECIFIERS IN ACTION\033[0m\n\n");

    // Integer outputs
    printf("\033[1;34m[Integer Stats]\033[0m\n");
    printf("  Kills      : %d (signed integer)\n", kills);
    printf("  Assists    : %d (signed integer)\n", assists);
    printf("  Deaths     : %d (signed integer)\n", deaths);
    printf("  Headshots  : %u (unsigned integer)\n", headshots);

    // Floating-point outputs
    printf("\n\033[1;36m[Floating-Point Stats]\033[0m\n");
    printf("  Accuracy   : %.2f%% (float, 2 decimal precision)\n", accuracy);
    printf("  KD Ratio   : %.3lf (double, 3 decimal precision)\n", kd_ratio);

    // Character and string outputs
    printf("\n\033[1;33m[Character and String]\033[0m\n");
    printf("  Rank       : %c (single character)\n", rank);
    printf("  Player Tag : %s (string)\n", player_tag);

    // Pointer output
    printf("\n\033[1;35m[Pointer]\033[0m\n");
    printf("  Kills Memory Location : %p (pointer address)\n", kills_ptr);

    printf("\033[1;31m\n// GGWP: Mastered format specifiers, now dominating the leaderboard!\033[0m\n");

    return 0; // No errors, just pure victory
}
