#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <CL/cl.h>
#include <string.h>

float* M;
float* V;
float* R_opencl;
float* R_seq;
int Width;
int Num_Threads;

const float delta = 0.0001;

// fill f with random float values
void fill(float* f, int size) {
  srand(time(NULL));
  for (int i = 0; i < size; i++)
    f[i] = ((float)rand()) / RAND_MAX;
}

// compares every pair lhs[i] and rhs[i] for i < width
void compare(float* lhs, float* rhs, int width) {
  int errors = 0;
  for (int i = 0; i < width; i++) {
    if (fabs(lhs[i] - rhs[i]) >= 0.0001) {
      printf("%f : %f\n", lhs[i], rhs[i]);
      errors += 1;
    }
  }
  if (errors > 0)
    printf("%d errors occurred.\n", errors);
  else
    printf("no errors occurred.\n");
}

// Sequential matrix-vector multiplication
void MatrixVecMulSeq() {
  for (int Row = 0; Row < Width; ++Row) {
    float sum = 0;
    for (int k = 0; k < Width; k++) {
      sum += M[Row * Width + k] * V[k];
    }
    R_seq[Row] = sum;
  }
}

// OpenCL section
cl_platform_id platform;
cl_device_id device;
cl_context context;
cl_command_queue commandQueue;
cl_kernel kernel;

void checkError(cl_int err) {
  if (err != CL_SUCCESS)
    printf("Error with errorcode: %d\n", err);
}

void initOpenCL() {
  cl_int err;

  err = clGetPlatformIDs(1, &platform, NULL);
  checkError(err);
  printf("platform selected\n");

  err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, NULL);
  checkError(err);
  printf("device selected\n");

  context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
  checkError(err);
  printf("context created\n");

  commandQueue = clCreateCommandQueue(context, device, 0, &err);
  checkError(err);
  printf("commandQueue created\n");
}

void printBuildLog(cl_program program, cl_device_id device) {
  cl_int err;
  char* build_log;
  size_t build_log_size;
  err = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &build_log_size);
  checkError(err);

  build_log = (char*) malloc(build_log_size);

  err = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, build_log_size, build_log, NULL);
  checkError(err);

  printf("Log:\n%s\n", build_log);

  free(build_log);
}

void makeKernel() {
  cl_int err;
  const char* kernelSource = "__kernel \
  void MatrixVecMultKernel(__global float* Md, \
                          __global float* Vd, \
                          __global float* Rd, int width) { \
    int row = get_global_id(0); \
    float sum = 0; \
    for (int k = 0; k < width; k++) \
      sum += Md[row * width + k] * Vd[k]; \
    Rd[row] = sum; \
  }";
  
  size_t sourceLength = strlen(kernelSource);
  cl_program program;
  program = clCreateProgramWithSource(context, 1, &kernelSource, &sourceLength, &err);
  checkError(err);
  printf("program created\n");

  err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
  if (err != CL_SUCCESS)
    printBuildLog(program, device);
  else
    printf("program built successfully\n");

  kernel = clCreateKernel(program, "MatrixVecMultKernel", &err);
  checkError(err);
  printf("kernel created\n");
}

void MatrixVecMulOpenCL(float* M, float* V, float* R, int width) {
  cl_int err;
  int matrixSize = width * width * sizeof(float);
  int vectorSize = width * sizeof(float);

  cl_mem Md = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, matrixSize, M, &err);
  checkError(err);
  printf("buffer Md created and copied\n");

  cl_mem Vd = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, vectorSize, V, &err);
  checkError(err);
  printf("buffer Vd created and copied\n");

  cl_mem Rd = clCreateBuffer(context, CL_MEM_READ_WRITE, vectorSize, NULL, &err);
  checkError(err);
  printf("buffer Rd created and memory allocated\n");

  err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &Md);
  err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &Vd);
  err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &Rd);
  err |= clSetKernelArg(kernel, 3, sizeof(int), &width);
  checkError(err);
  printf("kernel arguments set\n");

  size_t globalSize[] = {width};
  err = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, globalSize, NULL, 0, NULL, NULL);
  checkError(err);
  printf("enqueued kernel\n");

  err = clEnqueueReadBuffer(commandQueue, Rd, CL_TRUE, 0, vectorSize, R, 0, NULL, NULL);
  checkError(err);
  printf("enqueued read buffer Rd\n");
}

void init() {
  Width = 1024;
  M = (float*)malloc(Width * Width * sizeof(float));
  V = (float*)malloc(Width * sizeof(float));
  R_opencl = (float*)malloc(Width * sizeof(float));
  R_seq = (float*)malloc(Width * sizeof(float));

  fill(M, Width * Width);
  fill(V, Width);
  initOpenCL();
  makeKernel();
};

int main(void) {
  struct timeval start, end;
  init();

  gettimeofday(&start, NULL);
  MatrixVecMulOpenCL(M, V, R_opencl, Width);
  gettimeofday(&end, NULL);
  printf("Time elapsed OpenCL: %fmsecs\n",
    (float) (1000.0 * (end.tv_sec - start.tv_sec) + 0.001 * (end.tv_usec - start.tv_usec)));

  gettimeofday(&start, NULL);
  MatrixVecMulSeq();
  gettimeofday(&end, NULL);
  printf("Time elapsed Seq: %fmsecs\n",
    (float) (1000.0 * (end.tv_sec - start.tv_sec) + 0.001 * (end.tv_usec - start.tv_usec)));

  compare(R_seq, R_opencl, Width);

  return 0;
}
