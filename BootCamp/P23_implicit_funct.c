#include <stdio.h>
// implicit declaration for functions
void boo();
int main(int argc, const char* argv[]) {
boo();
return 0;
}
void boo(){
printf("boo() was called\n");
}