#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
void* perform(void *arg)
{
int i;
int *n = (int *)arg;
printf("processing from thread\r\n");
for(i=0;i<(*n);i++)
{
printf("%d ",i);
}
printf("\r\n");
}
int main(int argc, char* argv[])
{
pthread_t thread;
int ret;
errno = 0;
int n = 10;
ret = pthread_create (&thread, NULL, perform, &n);
if (ret)
{
printf("\n pthread_create() failed with error [%s]\n",strerror(errno));return -1;
}
int c = getchar(); // hold app to exit
return 0;
}
