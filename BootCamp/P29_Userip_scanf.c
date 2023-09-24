#include <stdio.h>
void scanf_demo();
int main() {
scanf_demo();
return 0;
}
void scanf_demo(){
int num;
char c;
char city[15];
float dec;
printf("Please enter an integer value: ");
scanf("%d", &num );
// %c ignores space characters
printf("Please enter a character: ");
scanf(" %c", &c );
printf("Please enter a city name (no space): ");
scanf("%s", city );
printf("Please enter a decimal value: ");
scanf("%f", &dec );
printf("\n-----result-------\n");
printf("number = %d\n", num );
printf("character = %c\n", c );
printf("city name = %s\n", city );
printf("decimal number = %f\n", dec );
}
