#include <stdio.h>

#include <pthread.h>

#define NUM_THREADS (5)

struct ThreadData {
    int (*op)(int, int);
    int *data;
    int len;
    int result;
};


int max(int a, int b)
{
  return (a>b) ? a : b;
}

int sum(int a, int b)
{
  return a+b;
}

int reduce(int (*op)(int, int),
           int *data,
           int len)
{
  int i;
  int result = data[0];
  for (i = 1; i < len; ++i)
    result = op(result, data[i]);
  return result;
}

void *thread_reduce(void *threadArg) {
    struct ThreadData *data = (struct ThreadData *)threadArg;
    // reuse sequential reduce implementation
    data->result = reduce(data->op, data->data, data->len);
    pthread_exit(NULL);
}

int parallel_reduce(int (*op)(int, int), 
                    int *data, 
                    int len) 
{
    pthread_t threads[NUM_THREADS];  // generate identifier for each thread
    struct ThreadData threadData[NUM_THREADS];  // generate ThreadData for each Thread
    int i;

    /* 
    Split data among threads
    Each Thread obtains its own sub array as parameter
    */
    int chunkSize = len / NUM_THREADS;
    for (i = 0; i < NUM_THREADS; ++i) {
        threadData[i].op = op;
        threadData[i].data = data + i * chunkSize;
        threadData[i].len = (i == NUM_THREADS - 1) ? (len - i * chunkSize) : chunkSize;
        // create thread
        pthread_create(&threads[i], NULL, thread_reduce, (void *)&threadData[i]);
    }

    // wait on termination of all threads
    for (i = 0; i < NUM_THREADS; ++i)
        pthread_join(threads[i], NULL);

    // concatenate results
    int result = threadData[0].result;
    for (i = 1; i < NUM_THREADS; ++i)
        result = op(result, threadData[i].result);
    
    return result;
}

int main() {
  int data[] = {1,2,3,4,5,6,7,8,9,10};
  int arr_len = *(&data + 1) - data;

  int m  = reduce(max, data, 10);
  int s = reduce(sum, data, 10);

  printf("max : %i; sum: %i\n", m, s);

  int pm = parallel_reduce(max, data, arr_len);
  int ps = parallel_reduce(sum, data, arr_len);

  printf("parallel max : %i; parallel sum: %i\n", pm, ps);
  return 0;
}

