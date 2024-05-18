#include <OpenCL/opencl.h>
#include <stdio.h>

int main() {
    cl_platform_id platform;
    cl_uint num_platforms;

    clGetPlatformIDs(1, &platform, &num_platforms);
    if (num_platforms > 0) {
        char platform_name[128];
        clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
        printf("OpenCL platform found: %s\n", platform_name);
    } else {
        printf("No OpenCL platform found.\n");
    }

    return 0;
}
