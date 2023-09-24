#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define BUFF_LEN 30

int buffer[BUFF_LEN];
int index_put=0, index_get=0;
int count = 0;
pthread_cond_t empty, get;
pthread_mutex_t lock;

void *producer(void *arg)
{
    int i;
    int *loops = (int *)arg;

    for (i = 0; i < (*loops); i++)
    {
        pthread_mutex_lock(&lock);
        while (count == BUFF_LEN)
            pthread_cond_wait(&empty, &lock);

        buffer[index_put] = i;
        index_put = (index_put + 1) % BUFF_LEN;
        count++;

        pthread_cond_broadcast(&get); //broadcast to consumer
        pthread_mutex_unlock(&lock);
    }
    printf("exit from producer\r\n");
}

void *consumer(void *arg)
{
    int i;
    int *loops = (int *)arg;
    pthread_t tid;

    tid = pthread_self();
    for (i = 0; i < (*loops); i++)
    {
        pthread_mutex_lock(&lock);
        while (count == BUFF_LEN)
            pthread_cond_wait(&get, &lock);

        int tmp = buffer[index_get];
        index_get = (index_get + 1) % BUFF_LEN;
        count--;
        printf("Thread consumer id: %ld. Value: %d\r\n", tid, tmp);

        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&lock);
    }
    printf("exit from consumer\r\n");
}

int main(int argc, char* argv[])
{
    pthread_t thread_consumer[2], thread_producer;
    int ret;
    errno = 0;
    int loops = 15;

    // initialization
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n pthread_mutex_init() failed with error [%s]\n",strerror(errno));
        return 1;
    }
    if (pthread_cond_init(&empty, NULL) != 0)
    {
        printf("\n pthread_cond_init() failed with error [%s]\n",strerror(errno));
        return 1;
    }
    if (pthread_cond_init(&get, NULL) != 0)
    {
        printf("\n pthread_cond_init() failed with error [%s]\n",strerror(errno));
        return 1;
    }

    // create thread
    ret = pthread_create (&thread_consumer[0], NULL, consumer, &loops);
    if (ret)
    {
      printf("\n pthread_create() failed with error [%s]\n",strerror(errno));
      return -1;
    }
    ret = pthread_create (&thread_consumer[1], NULL, consumer, &loops);
    if (ret)
    {
      printf("\n pthread_create() failed with error [%s]\n",strerror(errno));
      return -1;
    }
    ret = pthread_create (&thread_producer, NULL, producer, &loops);
    if (ret)
    {
      printf("\n pthread_create() failed with error [%s]\n",strerror(errno));
      return -1;
    }

    int c = getchar(); // hold app to exit
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&empty);
    pthread_cond_destroy(&get);

    return 0;
}
