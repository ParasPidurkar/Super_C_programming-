#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


pthread_t thread[5];
int total_finished_jobs;
pthread_mutex_t lock;

void* perform_job(void *arg)
{
  int i;
  pthread_t tid;

  tid = pthread_self();
  printf("processing from thread %ld \r\n",tid);
  printf("Thread %ld. Started Job. \r\n",tid);

  // do something for doing job
  for(i=0;i<20;i++)
  {
    sleep(1);
  }

  // update job counter
  pthread_mutex_lock(&lock);
  total_finished_jobs++;
  printf("Thread %ld. Finished Job. \r\n",tid);
  printf("Total current finished job: %d \r\n",total_finished_jobs);
  pthread_mutex_unlock(&lock);

}

int main(int argc, char* argv[])
{
    int i, ret;
    errno = 0;
    total_finished_jobs = 0;

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n pthread_mutex_init() failed with error [%s]\n",strerror(errno));
        return 1;
    }

    for(i=0;i<5;i++)
    {
        ret = pthread_create (&thread[i], NULL, perform_job, NULL);
        if (ret)
        {
          printf("\n pthread_create() failed with error [%s]\n",strerror(errno));
          return -1;
        }
    }

    int c = getchar(); // hold app to exit
    pthread_mutex_destroy(&lock);

    return 0;
}
