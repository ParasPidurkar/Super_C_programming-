#include <stdio.h>
// implicit declaration for functions
int add(int a, int b);
int main(int argc, const char* argv[]) {
int result = add(10,5);
printf("result: %d\n",result);
return 0;
}
int add(int a, int b){
return a + b;
}
