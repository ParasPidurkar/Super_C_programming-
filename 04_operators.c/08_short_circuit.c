#include <stdio.h>
int print(int i) {
printf("print function %d\n", i);
return i;
}
int main(void) {
int a = 20;
/* here 'print(a)' is not called,
since a 'a != 20' is false. */
if (a != 20 && print(a)) {
printf("I won't be printed!\n");
}
/* here 'print(a)' is called,
since a 'a == 20' is true. */
if (a == 20 && print(a)) {
printf("I will be printed!\n");
}
return 0;
}