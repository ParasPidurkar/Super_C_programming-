#include <stdio.h>
int main(void)
{
signed char c = 127; /* required to be 1 byte, see remarks for further information. */
signed short int si = 32767; /* required to be at least 16 bits. */
signed int i = 32767; /* required to be at least 16 bits */
signed long int li = 2147483647; /* required to be at least 32 bits. */
signed long long int lli = 2147483647; /* required to be at least 64 bits */

    printf("Size of signed char: %zu bytes\n", sizeof(c));
    printf("Size of signed short int: %zu bytes\n", sizeof(si));
    printf("Size of signed int: %zu bytes\n", sizeof(i));
    printf("Size of signed long int: %zu bytes\n", sizeof(li));
    printf("Size of signed long long int: %zu bytes\n", sizeof(lli));
return 0;
}