#include <stdio.h>
void getchar_putchar();
int main() {
getchar_putchar();
return 0;
}
void getchar_putchar(){
int c;
printf ("Type a character: ");
c = getchar();
printf ("char: %c\n",c);
putchar(c);
printf("\n");
}
