#include <stdio.h>
int main(int argc, const char* argv[]) {
int i;
FILE *f;
f = fopen("demo.txt", "w+");
for(i=0;i<5;i++){
fprintf(f, "fprintf message %d\n",i);
fputs("fputs message\n", f); // no format
}
fclose(f);
printf("Data was written into a file\n");
return 0;
}
