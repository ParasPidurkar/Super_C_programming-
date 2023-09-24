#include <stdio.h>
#include <string.h>
void concatenating();
int main(int argc, const char* argv[]) {
concatenating();
return 0;
}
void concatenating(){
printf("====concatenating===\n");
char str1[30] = "hello";
char str2[10] = "world";
strcat(str1,str2);
printf("result: %s\n",str1);
}