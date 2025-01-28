#include <stdio.h>


int func1(int num ,int arr[num]){
    int result =0;
    for(int i =0 ;i< num;i++)
    {
        result += arr[i];
    }
    return result;
    
}
int main()
{
    int ans =0;
    int arr1[5]={1,2,3,4,5};
    ans = func1(5,arr1);
    printf("%d",ans);

    return 0;
}
