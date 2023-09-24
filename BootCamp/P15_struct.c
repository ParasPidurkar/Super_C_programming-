#include <stdio.h>
// define a struct
struct employee{
int id;
char name[10];
char country[5];
};
int main() {
// declare struct variable
struct employee emp;
// set values
emp.id = 10;
sprintf(emp.name,"jane");
sprintf(emp.country,"DE");
// display
printf("id: %d, name: %s, country: %s\n",emp.id,emp.name,emp.country);
return 0;
}
