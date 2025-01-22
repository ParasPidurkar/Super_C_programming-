#include stdio.h
#include stdlib.h
#include time.h

 Using #define for constants
#define MAX_TRIES 5
#define MIN_NUMBER 1
#define MAX_NUMBER 100

 Using #define for macros
#define CLEAR_SCREEN() printf(033[H033[J)  ANSI escape sequence to clear the screen
#define RANDOM_NUMBER(min, max) ((rand() % ((max) - (min) + 1)) + (min))
#define PROMPT_USER(msg) { printf(%s, msg); fflush(stdout); }

void start_game() {
    int target, guess, attempts = 0;
    srand(time(0));  Seed random number generator
    target = RANDOM_NUMBER(MIN_NUMBER, MAX_NUMBER);

    CLEAR_SCREEN();
    printf(Welcome to the Number Guessing Game!n);
    printf(Guess a number between %d and %d. You have %d attempts.nn, MIN_NUMBER, MAX_NUMBER, MAX_TRIES);

    while (attempts  MAX_TRIES) {
        PROMPT_USER(Enter your guess );
        scanf(%d, &guess);

        attempts++;
        if (guess == target) {
            printf(🎉 Congratulations! You've guessed the correct number %dn, target);
            return;
        } else if (guess  target) {
            printf(Too low! Try again.n);
        } else {
            printf(Too high! Try again.n);
        }
        printf(Attempts remaining %dnn, MAX_TRIES - attempts);
    }
    printf(😞 You've used all your attempts. The correct number was %dn, target);
}

int main() {
    start_game();
    return 0;
}
