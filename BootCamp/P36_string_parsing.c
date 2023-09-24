#include <stdio.h>
#include <string.h>
void string_parser();
int main(int argc, const char* argv[]) {
string_parser();
return 0;
}
void string_parser(){
char cities[40] = "Tokyo;Berlin;London;New York";
char token[2]=";";  //char token[2] = {';', '\0'};
char* city;
printf("cities: %s\n",cities);
city = strtok(cities, token);
while(city != NULL){
printf("%s\n", city );
city = strtok(NULL, token);
/*This while loop prints each city and then tries to get the next city. The strtok function uses a static pointer internally to remember the last position it checked in the string. When it's called with NULL as the first argument, it continues from where it left off.*/
}
}