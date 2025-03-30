#include<stdio.h>
static const size_t N = 5;
int main()
{
size_t k = 0;
int arr[] = {1, 2, 3, 4, 5};
for(k = 0; k < N; k++)
{
printf("\n\t%d", *(arr + k));
}
return 0;
}