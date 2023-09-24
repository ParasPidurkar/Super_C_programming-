#include <stdio.h>
int main() {
// you can obtain input value from keyboard
// or any input device
int input = 3;
switch(input){
case 1:
printf("choosen 1\n");
break;
case 2:
printf("choosen 2\n");
break;
case 3:
case 4:
printf("choosen 3\n");
break;
}
return 0;
}
