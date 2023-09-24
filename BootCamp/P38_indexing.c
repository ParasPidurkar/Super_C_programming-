#include <stdio.h>
#include <string.h>
void string_explore();
int main(int argc, const char* argv[]) {
string_explore();
return 0;
}
void string_explore(){
char str[15] = "Hello world";
int index;
for(index=0;index<strlen(str);index++){
printf("%c\n",str[index]);
}
}
