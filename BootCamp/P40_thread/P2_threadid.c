#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
void* perform(void *arg)
{
int i;
int *n = (int *)arg;
pthread_t tid;
printf("processing from thread\r\n");
/* get the calling thread's ID */
tid = pthread_self();
printf("Thread id: %d \r\n",(int)tid);
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