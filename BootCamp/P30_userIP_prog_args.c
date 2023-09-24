#include <stdio.h>
int main(int argc, const char* argv[]) {
int i;
printf("total argument: %d\n",argc-1);
if(argc>1){
for(i=1;i<argc;i++){
printf("%s\n",argv[i]);
}
}
return 0;
}
