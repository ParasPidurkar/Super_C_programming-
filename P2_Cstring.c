#include <stdio.h>
#include<string.h>

int main()
{
    char name[20]={"Paras"};
    char last_name[20];
    strcpy(last_name,"Uchiha Madara");
    strcat(name," Pidurkar");
    printf("%s\n%s",name,last_name);

    return 0;
}
