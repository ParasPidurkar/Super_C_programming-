#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>


void* compute(void *arg)
{
  int i;
  pthread_t tid;
  double result = 0;

  tid = pthread_self();
  printf("processing from thread %ld \r\n",tid);

  // do something
  for(i=0;i<50;i++)
  {
    result = result + sin(i/2);
  }
  printf("Thread %ld. Completed, result=%e \r\n",tid,result);

  pthread_exit((void *)0);
}

int main(int argc, char* argv[])
{
    pthread_t thread[5];
    pthread_attr_t attr;
    void *status;
    int i, ret;
    errno = 0;

    // thread attribute
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for(i=0;i<5;i++)
    {
        ret = pthread_create (&thread[i], &attr, compute, NULL);
        if (ret)
        {
          printf("\n pthread_create() failed with error [%s]\n",strerror(errno));
          return -1;
        }
    }
    sleep(2);

    // joining thread
    printf("joining thread\r\n");
    for(i=0;i<5;i++)
    {
        ret = pthread_join(thread[i], &status);
        if (ret)
        {
          printf("\n pthread_join() failed with error [%s]\n",strerror(errno));
          return -1;
        }
        printf("Completed join with thread %d. Status: %ld\n", i,(long int)status);
    }

    int c = getchar(); // hold app to exit

    return 0;
}
