#include <stdio.h>
int main(void)
{
struct MyStruct
{
int x;
int y;
};
struct MyStruct myObject;
// myObject.x = 42;
// myObject.y = 123;
// printf(".x = %i, .y = %i\n", myObject.x, myObject.y); /* Outputs ".x = 42, .y = 123". */


struct MyStruct *p = &myObject;
p->x = 42;
p->y = 123;
printf(".x = %i, .y = %i\n", p->x, p->y); /* Outputs ".x = 42, .y = 123". */
printf(".x = %i, .y = %i\n", myObject.x, myObject.y); /* Also outputs ".x = 42, .y = 123". */
}