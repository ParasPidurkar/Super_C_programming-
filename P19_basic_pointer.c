#include <stdio.h>
int main(int argc, const char* argv[]) {
int n;
int* nPtr;
n = 10;
nPtr = &n;
printf("value n: %d \n",n);
printf("address n: %x \n",(unsigned int)&n);
printf("value nPtr: %x \n",(unsigned int)nPtr);
printf("address nPtr: %x \n",(unsigned int)&nPtr);
printf("value pointer nPtr: %d \n",*nPtr);
return 0;
}