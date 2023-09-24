#include <stdio.h>
int main() {
// define Multidimensional demenarray
int matrix[3][5];
// insert data
int i,j;
for(i=0;i<3;i++){
for(j=0;j<5;j++){
matrix[i][j] = i+j;
}
}
// display data
for(i=0;i<3;i++){
for(j=0;j<5;j++){
printf("%d ",matrix[i][j]);
}
printf("\n");
}
return 0;
}