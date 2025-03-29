#include <stdio.h>
#include <limits.h>

int main(void)
{
    printf("\033[1;31m// INTEGER LIMITS: ABSOLUTE POWER (KNOW YOUR BOUNDS)\033[0m\n\n");

    // CHAR LIMITS
    printf("\033[1;34m[CHARACTER]\033[0m\n");
    printf("  CHAR_BIT   : %d bits (smallest addressable unit)\n", CHAR_BIT);
    printf("  SCHAR_MIN  : %d (signed char min)\n", SCHAR_MIN);
    printf("  SCHAR_MAX  : %d (signed char max)\n", SCHAR_MAX);
    printf("  UCHAR_MAX  : %u (unsigned char max)\n", UCHAR_MAX);
    printf("  CHAR_MIN   : %d (char min, implementation-defined)\n", CHAR_MIN);
    printf("  CHAR_MAX   : %d (char max, implementation-defined)\n", CHAR_MAX);

    // SHORT LIMITS
    printf("\n\033[1;36m[SHORT]\033[0m\n");
    printf("  SHRT_MIN   : %d (short int min)\n", SHRT_MIN);
    printf("  SHRT_MAX   : %d (short int max)\n", SHRT_MAX);
    printf("  USHRT_MAX  : %u (unsigned short int max)\n", USHRT_MAX);

    // INT LIMITS
    printf("\n\033[1;35m[INTEGER]\033[0m\n");
    printf("  INT_MIN    : %d (int min)\n", INT_MIN);
    printf("  INT_MAX    : %d (int max)\n", INT_MAX);
    printf("  UINT_MAX   : %u (unsigned int max)\n", UINT_MAX);

    // LONG LIMITS
    printf("\n\033[1;33m[LONG]\033[0m\n");
    printf("  LONG_MIN   : %ld (long int min)\n", LONG_MIN);
    printf("  LONG_MAX   : %ld (long int max)\n", LONG_MAX);
    printf("  ULONG_MAX  : %lu (unsigned long int max)\n", ULONG_MAX);

    // LONG LONG LIMITS
    printf("\n\033[1;32m[LONG LONG]\033[0m\n");
    printf("  LLONG_MIN  : %lld (long long int min)\n", LLONG_MIN);
    printf("  LLONG_MAX  : %lld (long long int max)\n", LLONG_MAX);
    printf("  ULLONG_MAX : %llu (unsigned long long int max)\n", ULLONG_MAX);

    printf("\033[1;31m\n// GGWP: Full limits unleashed. No overflow is safe!\033[0m\n");

    return 0; // Victory screen: No overflows detected!
}
