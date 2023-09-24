#include <stdio.h>
#include "lib.h"
int main(int argc, const char* argv[]) {
int a,b,c;
a = 5;
b = 3;
c = add(a,b);
printf("%d + %d = %d\n",a,b,c);
c = subtract(a,b);
printf("%d - %d = %d\n",a,b,c);
c = multiply(a,b);
printf("%d * %d = %d\n",a,b,c);
return 0;
}
