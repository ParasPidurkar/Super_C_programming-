#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

void* perform(void *arg)
{
  int n = 0;

  // I'm not ready to be canceled
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
  printf("processing from thread\r\n");
  while(1)
  {
    printf("%d ",n);
    n++;

    pthread_testcancel();
    if(n>5)
    {
        // I'm ready to be canceled
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }

    sleep(1);
  }
  printf("\r\n");

}

int main(int argc, char* argv[])
{
    pthread_t thread;
    int ret, status;
    errno = 0;

    ret = pthread_create (&thread, NULL, perform, NULL);
    if (ret)
    {
      printf("\n pthread_create() failed with error [%s]\n",strerror(errno));
      return -1;
    }
    sleep(10);

    errno = 0;
    status = pthread_cancel(thread);
    if (status)
    {
        printf("\n pthread_cancel() failed with error [%s]\n",strerror(errno));
        return -1;
    }

    int c = getchar(); // hold app to exit
    return 0;
}
