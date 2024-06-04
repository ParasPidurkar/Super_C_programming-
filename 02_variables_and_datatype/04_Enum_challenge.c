
/*Gaming Enum Challenge: "Monster Hunter's Quest"

In the game "Monster Hunter's Quest," players embark on daring adventures to hunt down legendary creatures across vast landscapes. Each monster has its own unique strengths and weaknesses. Assume you're developing this game and need to create an enum to represent different types of monsters. Here's the challenge:

    Define an enum called MonsterType with the following constants: DRAGON, GIANT, UNDEAD, BEAST, and ELEMENTAL.

    Write a function printMonsterTypeDetails that takes a MonsterType parameter and prints out a description of the monster type. For example:
        If the parameter is DRAGON, it should print "Dragons are fierce creatures with powerful breath attacks."
        If the parameter is GIANT, it should print "Giants are towering beings known for their immense strength."
        If the parameter is UNDEAD, it should print "Undead monsters are reanimated corpses, often immune to traditional weapons."
        If the parameter is BEAST, it should print "Beasts come in various forms, from agile predators to hulking brutes."
        If the parameter is ELEMENTAL, it should print "Elemental monsters wield the power of nature's elements, such as fire, water, or thunder."

    Write a main function that demonstrates the usage of the MonsterType enum and the printMonsterTypeDetails function. Prompt the user to input a number corresponding to a MonsterType, then call printMonsterTypeDetails with the corresponding enum value.*/
#include <stdio.h>


enum MonsterType {
    DRAGON,
    GIANT,
    UNDEAD,
    BEAST,
    ELEMENTAL
};

void printMonsterTypeDetails(enum MonsterType type) {
    switch(type) {
        case DRAGON:
            printf("Dragons are fierce creatures with powerful breath attacks.\n");
            break;
        case GIANT:
            printf("Giants are towering beings known for their immense strength.\n");
            break;
        case UNDEAD:
            printf("Undead monsters are reanimated corpses, often immune to traditional weapons.\n");
            break;
        case BEAST:
            printf("Beasts come in various forms, from agile predators to hulking brutes.\n");
            break;
        case ELEMENTAL:
            printf("Elemental monsters wield the power of nature's elements, such as fire, water, or thunder.\n");
            break;
        default:
            printf("Invalid monster type.\n");
    }
}

int main() {
    int choice;

    // Prompt the user to choose a monster type
    printf("Choose a monster type:\n");
    printf("1. Dragon\n");
    printf("2. Giant\n");
    printf("3. Undead\n");
    printf("4. Beast\n");
    printf("5. Elemental\n");
    printf("Enter your choice: ");
    scanf("%d", &choice);

    // Convert the user's choice to the corresponding enum value
    enum MonsterType selectedType;
    switch(choice) {
        case 1:
            selectedType = DRAGON;
            break;
        case 2:
            selectedType = GIANT;
            break;
        case 3:
            selectedType = UNDEAD;
            break;
        case 4:
            selectedType = BEAST;
            break;
        case 5:
            selectedType = ELEMENTAL;
            break;
        default:
            printf("Invalid choice.\n");
            return 1; // Exit the program with an error code
    }

    // Print the details of the selected monster type
    printMonsterTypeDetails(selectedType);

    return 0;
}
