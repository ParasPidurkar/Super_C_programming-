#include <stdio.h>
// implicit declaration for functions
double mean(int numbers[],int size);
int main(int argc, const char* argv[]) {
int numbers[8] = {8,4,5,1,4,6,9,6};
double ret_mean = mean(numbers,8);
printf("mean: %.2f\n",ret_mean);
return 0;
}
double mean(int numbers[],int size){
int i, total;
double temp;
for (i = 0; i < size; ++i){
total += numbers[i];
}
temp = (double)total / (double)size;
return temp;
}
