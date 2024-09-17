#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>
#include <vector>
#include <CL/cl.hpp>

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} Pixel;

void writePPM(Pixel* pixels, const char* filename, int width, int height) {
    std::ofstream outputFile(filename, std::ios::binary);

    // write header:
    outputFile << "P6\n" << width << " " << height << "\n255\n";

    outputFile.write((const char*)pixels, sizeof(Pixel) * width * height);
}

Pixel* readPPM(const char* filename, int* width, int* height) {
    std::ifstream inputFile(filename, std::ios::binary);

    // parse header
    inputFile.ignore(2, '\n'); // ignore P6
    while (inputFile.peek() == '#') { inputFile.ignore(1024, '\n'); } // skip comment
    inputFile >> (*width);
    inputFile.ignore(1, ' '); // ignore space
    inputFile >> (*height);
    inputFile.ignore(1, '\n'); // ignore newline
    while (inputFile.peek() == '#') { inputFile.ignore(1024, '\n'); } // skip comment
    inputFile.ignore(4, '\n'); // ignore 255 and newline

    Pixel* data = (Pixel*)malloc(sizeof(Pixel) * (*width) * (*height));

    inputFile.read((char*)data, sizeof(Pixel) * (*width) * (*height));

    return data;
}

const char* kernelSource = R"CLC(
__kernel void gaussFilter(__global Pixel* image, __global Pixel* output, __constant float* weights, int width, int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= 2 && y >= 2 && x < width - 2 && y < height - 2) {
        float4 sum = (float4)(0.0f);
        for (int i = -2; i <= 2; i++) {
            for (int j = -2; j <= 2; j++) {
                Pixel p = image[(y + j) * width + (x + i)];
                sum.x += p.r * weights[(i + 2) * 5 + (j + 2)];
                sum.y += p.g * weights[(i + 2) * 5 + (j + 2)];
                sum.z += p.b * weights[(i + 2) * 5 + (j + 2)];
            }
        }
        output[y * width + x].r = (unsigned char)sum.x;
        output[y * width + x].g = (unsigned char)sum.y;
        output[y * width + x].b = (unsigned char)sum.z;
    }
}
)CLC";

void calculateWeights(float weights[5][5]) {
    float sigma = 1.0;
    float r, s = 2.0 * sigma * sigma;

    // sum is for normalization
    float sum = 0.0;

    // generate weights for 5x5 kernel
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            r = x * x + y * y;
            weights[x + 2][y + 2] = exp(-(r / s)) / (M_PI * s);
            sum += weights[x + 2][y + 2];
        }
    }

    // normalize the weights
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            weights[i][j] /= sum;
        }
    }
}

int main(int argc, char** argv) {
    const char* inFilename = (argc > 1) ? argv[1] : "lena.ppm";
    const char* outFilename = (argc > 2) ? argv[2] : "output.ppm";

    float weights[5][5];
    calculateWeights(weights);
    int width, height;

    Pixel* image = readPPM(inFilename, &width, &height);
    Pixel* output = (Pixel*)malloc(sizeof(Pixel) * width * height);

    try {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        cl::Platform platform = platforms.front();
        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

        cl::Device device = devices.front();
        cl::Context context({ device });

        cl::Program::Sources sources;
        sources.push_back({ kernelSource, strlen(kernelSource) });

        cl::Program program(context, sources);
        program.build({ device });

        cl::Buffer bufferImage(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Pixel) * width * height, image);
        cl::Buffer bufferOutput(context, CL_MEM_WRITE_ONLY, sizeof(Pixel) * width * height);
        cl::Buffer bufferWeights(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * 25, weights);

        cl::Kernel kernel(program, "gaussFilter");
        kernel.setArg(0, bufferImage);
        kernel.setArg(1, bufferOutput);
        kernel.setArg(2, bufferWeights);
        kernel.setArg(3, width);
        kernel.setArg(4, height);

        cl::CommandQueue queue(context, device);
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(width, height), cl::NullRange);
        queue.finish();

        queue.enqueueReadBuffer(bufferOutput, CL_TRUE, 0, sizeof(Pixel) * width * height, output);

        writePPM(output, outFilename, width, height);
    } catch (const cl::Error& e) {
        std::cerr << "OpenCL error: " << e.what() << "(" << e.err() << ")" << std::endl;
        return 1;
    }

    free(image);
    free(output);
    return 0;
}
