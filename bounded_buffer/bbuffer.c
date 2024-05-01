#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <pthread.h>

#define MAX_ITER (10)

struct Buffer {
  char *buf;
  int in;
  int out;
  int count;
  int size;
  // add mutex to buffer struct
  pthread_mutex_t mutex;
  // add condition to buffer struct
  pthread_cond_t not_full;
  pthread_cond_t not_empty;
};

void put(struct Buffer *b, char c) {
  /*
  Uses conditional sychronization with signal mechanism!
  */

  printf("put\n");
  // lock mutex, this thread waits until it obtains the mutex
  pthread_mutex_lock(&b->mutex);
  // while buffer is full, this thread should not proceed!
  while (b->count == b->size)
  {
    /*
    The following line
    1. frees the mutex (i.e., enables other get thread to release an item as we need space in buffer to put another item)
    2. waits until the condition gets true, then locks the mutex and proceeds with subsequent LOC
    */
    pthread_cond_wait(&(b->not_full), &(b->mutex));
  }
  // put item in buffer, increment counter
  b->buf[b->in] = c;
  b->in = ((b->in)+1)%(b->size);
    ++(b->count);
  // unblocks at least one of the threads that are blocked on the specified condition variable cond
  // (if any threads are blocked on cond).
  pthread_cond_signal(&(b->not_empty));
  /*
  NOTE: The function pthread_cond_signal is only correct if exactly one producer and consumer thread exist.
  Otherwise a deadlock can be generated (pthread_cond_broadcast instead).
  */
  // unlock mutex  
  pthread_mutex_unlock(&(b->mutex));
}

char get(struct Buffer *b) {
  /*
  Uses conditional sychronization with signal mechanism!
  */

  // lock mutex
  pthread_mutex_lock(&(b->mutex));
  // while buffer is empty, wait as we have no items left to get from the buffer
  while (b->count == 0)
  {
    /*
    The following line
    1. frees the mutex (i.e., enables other get thread to release an item as we need space in buffer to put another item)
    2. waits until the condition gets true, then locks the mutex and proceeds with subsequent LOC
    */
    pthread_cond_wait(&(b->not_empty), &(b->mutex));
  }
  printf("get\n");
  // get top item from buffer
  char c = b->buf[b->out];
  b->buf[b->out] = '\0';
  b->out = ((b->out)+1)%(b->size);
  --(b->count);
  // unblocks at least one of the threads that are blocked on the specified condition variable cond
  // (if any threads are blocked on cond).
  pthread_cond_signal(&(b->not_full));
  /*
  NOTE: The function pthread_cond_signal is only correct if exactly one producer and consumer thread exist.
  Otherwise a deadlock can be generated (pthread_cond_broadcast instead).
  */
  // unlock mutex
  pthread_mutex_unlock(&(b->mutex));
  return c;
}

void initBuffer(struct Buffer *b, int size) {
  b->in = 0; b->out = 0; b->count = 0;
  b->size = size;
  // allocate memory of size times bytes
  b->buf = (char*)malloc(sizeof(char) * size);
  // if memory allocation failed
  if (b->buf == NULL)
    // exit program with -1
    exit(-1);
  // init mutex
  pthread_mutex_init(&(b->mutex), NULL);
  // init conditions
  pthread_cond_init(&(b->not_empty), NULL);
  pthread_cond_init(&(b->not_full), NULL);
};

void destroyBuffer(struct Buffer* b) {
  free(b->buf);
  // destroy mutex and conditions
  pthread_mutex_destroy(&(b->mutex));
  pthread_cond_destroy(&(b->not_empty));
  pthread_cond_destroy(&(b->not_full));
};

int getRandSleepTime() {
  return rand() % 100 + 750000;
}

void *producerFunc(void *param) {
  struct Buffer* b = (struct Buffer*)param;
  int i;
  for (i = 0; i < MAX_ITER; ++i) {
    // get random small ascii character
    char c = (char)(26 * (rand() / (RAND_MAX + 1.0))) + 97;
    put(b, c);
    usleep( getRandSleepTime()*2 );
  }
  return NULL;
};

void *consumerFunc(void *param) {
  struct Buffer* b = (struct Buffer*)param;
  int i;
  for (i = 0; i < MAX_ITER; ++i) {
    printf("%c\n", get(b));
    usleep( getRandSleepTime());
  };
  return NULL;
};

int main(int argc, char **argv) {
  pthread_t producer, consumer;

  srand(time(NULL));

  struct Buffer b;
  initBuffer(&b, 10);

  pthread_create( &producer, NULL, producerFunc, &b );
  pthread_create( &consumer, NULL, consumerFunc, &b );

  // calling thread (main thread) waits on the termination of the target thread (producer and consumer threads)
  // as we do not need the return values of producer and consumer threads, we can pass NULL as the second function argument
  pthread_join( producer, NULL );
  pthread_join( consumer, NULL );

  destroyBuffer(&b);
  return 0;
}
