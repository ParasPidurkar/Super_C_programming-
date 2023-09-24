#include <stdio.h>
struct employee{
int id;
char name[10];
char country[5];
};
int main() {
// define array
int numbers[5];
char chars[10];
struct employee list[5];
// insert data
int i;
for(i=0;i<5;i++){
numbers[i] = i;
list[i].id = i;
sprintf(list[i].name,"usr %d",i);
sprintf(list[i].country,"DE");
}
sprintf(chars,"hello c");
// display data
for(i=0;i<5;i++){
printf("%d %c\n",numbers[i],chars[i]);
printf("struct. id: %d, name: %s, country: %s \n",
list[i].id,list[i].name,list[i].country);
}
printf("%s\n",chars);
return 0;
}
