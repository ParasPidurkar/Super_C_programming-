#include <stdio.h>


int main(void)
{
    int d = 42;     // decimal constant (base 10)
    int o = 052;    // octal constant (base 8)
    int x = 0xaf;   // hexadecimal constant (base 16, lowercase)
    int X = 0XAf;   // hexadecimal constant (base 16, uppercase)

    
    printf("Decimal (d): %d\n", d);
    printf("Octal (o): %d\n", o);
    printf("Hexadecimal lowercase (x): %d\n", x);
    printf("Hexadecimal uppercase (X): %d\n", X);

    printf("Decimal (d): %d\n", d);
    printf("Octal (o): %o\n", o);
    printf("Hexadecimal lowercase (x): %x\n", x);
    printf("Hexadecimal uppercase (X): %X\n", X);

    return 0; // GGWP
}
