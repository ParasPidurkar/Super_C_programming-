#include <stdio.h>
#include <stdlib.h>
#include <math.h>
int main() {
float a;
float b;
float pi = 3.14;
a = 0.25;
printf("value a = %.2f \n",a);
b = abs(-a);
printf("abs(a)=%.2f \n",b);
b = acos(a);
printf("acos(a)=%.2f \n",b);
b = asin(a);
printf("asin(a)=%.2f \n",b);
b = atan(a);
printf("abs(a)=%.2f \n",b);
b = atan2(a,5);
printf("atan(a,5)=%.2f \n",b);
b = cos(a);
printf("cos(a)=%.2f \n",b);
b = sin(a);
printf("sin(a)=%.2f \n",b);
b = tan(a);
printf("tan(a)=%.2f \n",b);
b = sqrt(a);
printf("sqrt(a)=%.2f \n",b);
return 0;
}
