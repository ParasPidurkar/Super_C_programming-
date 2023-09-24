#include <stdio.h>
void gets_puts();
int main() {
gets_puts();
return 0;
}
void gets_puts(){
char name[256];
printf("-----fget() input-----\n");
name[0] = '\0'; // clear
printf ("Your name: ");
fgets(name,256,stdin);
printf ("name: %s\n",name);
puts(name);
}
