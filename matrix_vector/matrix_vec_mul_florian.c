#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

// Größe der Matrix und des Vektors
#define N 4

// OpenCL Kernel als String
const char* kernelSource = "\n" \
"__kernel void matvec_mult(__global float* M, __global float* V, __global float* R, int N) {\n" \
"    int i = get_global_id(0);\n" \
"    float sum = 0.0;\n" \
"    for (int j = 0; j < N; j++) {\n" \
"        sum += M[i * N + j] * V[j];\n" \
"    }\n" \
"    R[i] = sum;\n" \
"}\n";

int main() {
    // Matrix und Vektor definieren und initialisieren
    float M[N*N] = {1, 2, 3, 4,
                    5, 6, 7, 8,
                    9, 10, 11, 12,
                    13, 14, 15, 16};
    float V[N] = {1, 1, 1, 1};
    float R[N] = {0};

    // OpenCL Initialisierung
    cl_platform_id platform_id;
    cl_device_id device_id;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem d_M, d_V, d_R;
    size_t global[1] = {N};
    int err;

    // Plattform und Gerät auswählen
    clGetPlatformIDs(1, &platform_id, NULL);
    clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);

    // Kontext und Befehlswarteschlange erstellen
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    queue = clCreateCommandQueue(context, device_id, 0, &err);

    // Speicher auf dem Gerät zuweisen
    d_M = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * N * N, M, &err);
    d_V = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * N, V, &err);
    d_R = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * N, NULL, &err);

    // Programm aus dem Kernel-Source erstellen und kompilieren
    program = clCreateProgramWithSource(context, 1, &kernelSource, NULL, &err);
    clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

    // Kernel erstellen
    kernel = clCreateKernel(program, "matvec_mult", &err);

    // Kernel-Argumente setzen
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_M);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_V);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_R);
    int size = N;
    clSetKernelArg(kernel, 3, sizeof(int), &size);

    // Kernel ausführen
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global, NULL, 0, NULL, NULL);

    // Ergebnis zurücklesen
    clEnqueueReadBuffer(queue, d_R, CL_TRUE, 0, sizeof(float) * N, R, 0, NULL, NULL);

    // Ergebnis ausgeben
    printf("Ergebnis:\n");
    for (int i = 0; i < N; i++) {
        printf("%f\n", R[i]);
    }

    // Speicher freigeben
    clReleaseMemObject(d_M);
    clReleaseMemObject(d_V);
    clReleaseMemObject(d_R);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return 0;
}
