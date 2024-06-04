#include <stdio.h>

int main() {
    char ch = 'A';
    int num = 123;
    long int longNum = 1234567890L;
    long long int longLongNum = 123456789012345LL;
    float floatNum = 3.14f;
    double doubleNum = 3.141592653589793;
    long double longDoubleNum = 3.141592653589793238L;
    char str[] = "Hello, World!";

    printf("Character: %c\n", ch);
    printf("Integer: %d\n", num);
    printf("Long Integer: %ld\n", longNum);
    printf("Long Long Integer: %lld\n", longLongNum);
    printf("Float: %f\n", floatNum);
    printf("Double: %lf\n", doubleNum);
    printf("Long Double: %Lf\n", longDoubleNum);
    printf("String: %s\n", str);

    // Printing pointers
    printf("Pointer: %p\n", &num);

    // Printing octal and hexadecimal
    printf("Octal: %o\n", num);
    printf("Hexadecimal: %x\n", num);

    // Printing characters with ASCII value
    printf("Character with ASCII value: %c, %d\n", ch, ch);

    return 0;
}
