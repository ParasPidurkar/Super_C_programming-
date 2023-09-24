#include <stdio.h>
int main(int argc, const char* argv[]) {
char ch;
FILE *f;
printf("Reading a file….\n");
f = fopen("demo.txt", "r");
if(f==NULL){
printf("Failed to read file\n");
return 0;
}
while((ch = fgetc(f)) != EOF )
printf("%c",ch);
fclose(f);
return 0;
}
