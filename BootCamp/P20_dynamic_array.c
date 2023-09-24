#include <stdio.h>
#include <stdlib.h>
int main(int argc, const char* argv[]) {
// define dynamic array of pointer
int *numbers; // single array pointer
// a number of array
int N = 10;
// allocate memory
numbers = malloc( N * sizeof(int));
// set values
int i;
for(i=0;i<N;i++){
numbers[i] = i+3;
}
// display values
for(i=0;i<N;i++){
printf("%d ",numbers[i]);
}
printf("\n");
// free memory
free(numbers);

// display values
for(i=0;i<N;i++){
printf("%d ",numbers[i]);
}
printf("\n");
return 0;
}