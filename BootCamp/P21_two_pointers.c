#include <stdio.h>
#include <stdlib.h>
int main(int argc, const char* argv[]) {
// define dynamic array of pointer
int **matrix; // two dimensional array pointer
// a number of array
int M = 3;
int N = 5;
// allocate memory
matrix = malloc( M * sizeof(int*));
// set values
int i,j;
for(i=0;i<M;i++){
matrix[i] = malloc( N * sizeof(int));
for(j=0;j<N;j++){
matrix[i][j] = i + j;
}
}
// display values
for(i=0;i<M;i++){
for(j=0;j<N;j++){
printf("%d ",matrix[i][j]);
}
printf("\n");
}
// free memory
free(matrix);
return 0;
}
