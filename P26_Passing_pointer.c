#include <stdio.h>
// implicit declaration for functions
void swap(int *px, int *py);
int main() {
int *x, *y;
int a, b;
a = 10;
b = 5;
// set value
x = &a;
y = &b;
printf("value pointer x: %d \n",*x);
printf("value pointer y: %d \n",*y);
swap(x,y);
printf("swap()\n");
printf("value pointer x: %d \n",*x);
printf("value pointer y: %d \n",*y);
return 0;
}
void swap(int *px, int *py){
int temp;
// store pointer px value to temp
temp = *px;
// set pointer px by py value
*px = *py;
// set pointer py by temp value
*py = temp;
}
