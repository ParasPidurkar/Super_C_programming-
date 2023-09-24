#include <stdio.h>
int main() {
int a,b,c;
a = 10;
b = 6;
c = a + b;
printf("%d + %d = %d\n",a,b,c);
c = a - b;
printf("%d - %d = %d\n",a,b,c);
c = a * b;
printf("%d * %d = %d\n",a,b,c);
float d = a / b;
printf("%d / %d = %.2f\n",a,b,d);
float e =(float)a / b;
printf("%d / %d = %.2f\n",a,b,e);
return 0;
}
