#include <stdio.h>

#include <pthread.h>

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

int parallel_reduce(int (*op)(int, int),
                    int *data,
                    int len)
{
}

int main() {
  int data[] = {1,2,3,4,5,6,7,8,9,10};

  int m  = reduce(max, data, 10);
  int s = reduce(sum, data, 10);

  printf("max : %i; sum: %i\n", m, s);

  int pm = parallel_reduce(max, data, 10);
  int ps = parallel_reduce(sum, data, 10);

  printf("parallel max : %i; parallel sum: %i\n", pm, ps);
  return 0;
}

