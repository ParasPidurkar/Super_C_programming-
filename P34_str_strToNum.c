#include <stdio.h>
#include <string.h>
void string_to_numeric();
int main(int argc, const char* argv[]) {
string_to_numeric();
return 0;
}
void string_to_numeric(){
printf("====string_to_numeric===\n");
char str1[10] = "10";
char str2[10] = "28.74";
int num1;
float num2;
sscanf(str1,"%d",&num1);
sscanf(str2,"%f",&num2);
printf("num1: %d\n",num1);
printf("num2: %f\n",num2);
}
