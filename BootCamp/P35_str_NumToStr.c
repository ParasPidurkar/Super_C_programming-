#include <stdio.h>
#include <string.h>
void numeric_to_string();
int main(int argc, const char* argv[]) {
numeric_to_string();
return 0;
}
void numeric_to_string(){
printf("====numeric_to_string===\n");
int n = 10;
float m = 23.78;
char num1[10];
char num2[10];
sprintf(num1,"%d",n);
sprintf(num2,"%.2f",m);
printf("num1: %s\n",num1);
printf("num2: %s\n",num2);
}
