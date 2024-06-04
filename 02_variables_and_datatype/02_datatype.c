#include <stdio.h>
#include <limits.h>
#include <float.h>

int main() {
    // Integer data types
    char charVar = 'A';
    short shortVar = 32767;
    int intVar = 2147483647;
    long longVar = 9223372036854775807L;
    long long longLongVar = 9223372036854775807LL;
    signed char signedCharVar = 127;
    unsigned char unsignedCharVar = 255;
    unsigned short unsignedShortVar = 65535;
    unsigned int unsignedIntVar = 4294967295;
    unsigned long unsignedLongVar = 18446744073709551615UL;
    unsigned long long unsignedLongLongVar = 18446744073709551615ULL;

    // Floating-point data types
    float floatVar = 3.40282347e38f;
    double doubleVar = 1.7976931348623158e308;
    long double longDoubleVar = 1.189731495357231765e4932L;

    // Print the initialized variables
    printf("Character: %c\n", charVar);
    printf("Short: %hd\n", shortVar);
    printf("Int: %d\n", intVar);
    printf("Long: %ld\n", longVar);
    printf("Long Long: %lld\n", longLongVar);
    printf("Signed Char: %hhd\n", signedCharVar);
    printf("Unsigned Char: %hhu\n", unsignedCharVar);
    printf("Unsigned Short: %hu\n", unsignedShortVar);
    printf("Unsigned Int: %u\n", unsignedIntVar);
    printf("Unsigned Long: %lu\n", unsignedLongVar);
    printf("Unsigned Long Long: %llu\n", unsignedLongLongVar);
    printf("Float: %f\n", floatVar);
    printf("Double: %lf\n", doubleVar);
    printf("Long Double: %Lf\n", longDoubleVar);

    return 0;
}
