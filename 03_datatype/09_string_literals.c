#include <stdio.h>
int main(void)
{
char* str = "hello, world"; /* string literal */
/* string literals can be used to initialize arrays */
char a1[] = "abc"; /* a1 is char[4] holding {'a','b','c','\0'} */
char a2[4] = "abc"; /* same as a1 */
char a3[3] = "abc"; /* a1 is char[3] holding {'a','b','c'}, missing the '\0' */
printf("a3  %s",a3);


char* s = "foobar";
s[0] = 'F'; /* undefined behaviour */
/* it's good practice to denote string literals as such, by using `const` */
char const* s1 = "foobar";
// s1[0] = 'F'; /* compiler error! */
return 0;
}