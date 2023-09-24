#include <stdio.h>
int main() {
int a,b;
a = 5;
b = 8;
printf("%d \n",a>b && a!=b);
printf("%d \n",!(a>=b));
printf("%d \n",a==b || a>b);
return 0;
}
