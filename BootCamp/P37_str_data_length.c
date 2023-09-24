#include <stdio.h>
#include <string.h>
void string_length();
int main(int argc, const char* argv[]) {
string_length();
return 0;
}
void string_length(){
char str[20] = "Hello world";
printf("str: %s\n",str);
printf("length: %d\n",(int)strlen(str));
}
