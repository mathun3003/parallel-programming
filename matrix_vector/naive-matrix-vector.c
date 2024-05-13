#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define N 32*32768LL
#define M 1024LL

int main(char *args[]) {
  double *y = calloc(M, sizeof(double));
  double *x = malloc(N * sizeof(double));
  double *m = malloc(M * N * sizeof(double)); // 8 GiB

  int i, j;
     
  clock_t start, end;
  double cpu_time_used;

  start = clock();
  for ( i = 0; i < M; ++i )
    for ( j = 0; j < N; ++j )
          y[i] = y[i] + m[i * N + j] * x[j];
  end = clock();

  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
  printf("Time taken: %f seconds\n", cpu_time_used);

  free(m);
  free(x);
  free(y);

  exit(EXIT_SUCCESS);
}
