#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define N 32*32768LL
#define M 1024LL

#define BLOCK 32LL

int main(char *args[]) {
  size_t n = N;
  size_t bn = BLOCK;
  size_t bm = 512;

  double *y = calloc(M, sizeof(double));
  double *x = malloc(N * sizeof(double));
  double *m = malloc(M * N * sizeof(double));

  int i, j, k, l;
     
  clock_t start, end;
  double cpu_time_used;

  start = clock();
  for ( i = 0; i < M / bm ; ++ i )
    for ( j = 0; j < N / bn ; ++ j )
      for ( k = i * bm ; k < ( i +1) * bm ; ++ k )
        for ( l = j * bn ; l < ( j +1) * bn ; ++ l )
          y[k] = y[k] + m[k * N + l] * x[l];
  end = clock();

  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
  printf("Time taken %u: %f seconds\n", bn, cpu_time_used);

  free(m);
  free(x);
  free(y);
  exit(EXIT_SUCCESS);
}
