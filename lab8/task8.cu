#include <stdio.h>
#include <iostream>
#include <cmath>
#include <fstream>
#include <chrono>
#include <boost/program_options.hpp>
#include <cublas_v2.h>
#include <cuda_runtime.h>
#include <cub/cub.cuh>

namespace po = boost::program_options;

void init_matrix(double* matrix, int n) {
    matrix[n+1] = 10.0;
    matrix[n - 1 + n - 1] = 20.0;
    matrix[(n - 1) * (n - 1)] = 20.0;
    matrix[(n - 1) * (n - 1) + (n - 1) - 2] = 30.0;

    for (int j = 2; j < n - 2; j++) {
        matrix[n + j] = matrix[n + 1] + (matrix[n - 1 + n - 1] - matrix[n + 1]) * (static_cast<double>(j-1) / static_cast<double>(n - 3));
        matrix[(n - 1) * (n - 1) + j - 1] = matrix[(n - 1) * (n - 1)] + (matrix[(n - 1) * (n - 1) + (n - 1) - 2] - matrix[(n - 1) * (n - 1)]) * (static_cast<double>(j-1) / static_cast<double>(n - 3));
        matrix[j * n + 1] = matrix[n + 1] + (matrix[(n - 1) * (n - 1)] - matrix[n + 1]) * (static_cast<double>(j-1) / static_cast<double>(n - 3));
        matrix[j * n + (n - 1) - 1] = matrix[n - 1 + n - 1] + (matrix[(n - 1) * (n - 1) + (n - 1) - 2] - matrix[n - 1 + n - 1]) * (static_cast<double>(j-1) / static_cast<double>(n - 3));
    }
}

__global__ void jacobi_kernel(double* src, double* dst, int width) {
    int i = blockIdx.y * blockDim.y + threadIdx.y + 1;
    int j = blockIdx.x * blockDim.x + threadIdx.x + 1;
    
    if (i < width - 1 && j < width - 1) {
        dst[i * width + j] = 0.25 * (src[i * width + j - 1] + 
                                    src[i * width + j + 1] + 
                                    src[(i - 1) * width + j] + 
                                    src[(i + 1) * width + j]);
    }
}

__global__ void diff_kernel(double* a, double* b, double* diff, int size) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        diff[idx] = fabs(a[idx] - b[idx]);
    }
}

double compute_max_diff(double* d_a, double* d_b, int size) {
    double* d_diff;
    cudaMalloc(&d_diff, size * sizeof(double));
    
    dim3 blockDim(256);
    dim3 gridDim((size + blockDim.x - 1) / blockDim.x);
    diff_kernel<<<gridDim, blockDim>>>(d_a, d_b, d_diff, size);
    cudaDeviceSynchronize();
    
    void* d_temp_storage = NULL;
    size_t temp_storage_bytes = 0;
    double max_diff;
    double* d_max_diff;
    cudaMalloc(&d_max_diff, sizeof(double));
    
    cub::DeviceReduce::Max(d_temp_storage, temp_storage_bytes, d_diff, d_max_diff, size);
    cudaMalloc(&d_temp_storage, temp_storage_bytes);
    cub::DeviceReduce::Max(d_temp_storage, temp_storage_bytes, d_diff, d_max_diff, size);
    cudaMemcpy(&max_diff, d_max_diff, sizeof(double), cudaMemcpyDeviceToHost);
    
    cudaFree(d_temp_storage);
    cudaFree(d_max_diff);
    cudaFree(d_diff);
    
    return max_diff;
}

int main(int argc, char** argv) {
    int n = 20;
    double accuracy = 1e-6;
    int max_iteration = 1000000;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "show help message")
        ("size", po::value<int>(&n), "grid size (128, 256, 512, 1024)")
        ("accuracy", po::value<double>(&accuracy), "desired accuracy")
        ("max_iter", po::value<int>(&max_iteration), "maximum number of iterations");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 0;
    }

    const int width = n + 2;
    const size_t matrix_size = width * width * sizeof(double);

    double* h_matrix1 = new double[width * width];
    double* h_matrix2 = new double[width * width];
    
    init_matrix(h_matrix1, width);
    init_matrix(h_matrix2, width);

    double *d_matrix1, *d_matrix2;
    cudaMalloc(&d_matrix1, matrix_size);
    cudaMalloc(&d_matrix2, matrix_size);

    cudaMemcpy(d_matrix1, h_matrix1, matrix_size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_matrix2, h_matrix2, matrix_size, cudaMemcpyHostToDevice);

    dim3 block_size(16, 16);
    dim3 grid_size((width + block_size.x - 1) / block_size.x, 
                   (width + block_size.y - 1) / block_size.y);

    int iter = 0;
    double error = 1.0;
    bool use_matrix1 = true;

    cudaGraph_t graph;
    cudaGraphExec_t graphExec;

    auto start = std::chrono::steady_clock::now();

    cudaStream_t stream;
    cudaStreamCreate(&stream);
    cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal);
    
    for (int i = 0; i < 500; i++) {
      if (use_matrix1) {
        jacobi_kernel<<<grid_size, block_size, 0, stream>>>(d_matrix1, d_matrix2, width);
      } else {
        jacobi_kernel<<<grid_size, block_size, 0, stream>>>(d_matrix2, d_matrix1, width);
      }
      use_matrix1 = !use_matrix1;
    }

    cudaStreamEndCapture(stream, &graph);
    cudaGraphInstantiate(&graphExec, graph, NULL, NULL, 0);
    cudaStreamDestroy(stream);

    while (error > accuracy && iter < max_iteration) {
      cudaGraphLaunch(graphExec, 0);
      cudaDeviceSynchronize();
      
      iter += 500;
      
      error = compute_max_diff(use_matrix1 ? d_matrix1 : d_matrix2, 
                              use_matrix1 ? d_matrix2 : d_matrix1, 
                              width * width);
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "Iterations: " << iter << "\n";
    std::cout << "Final error: " << error << "\n";
    std::cout << "Time elapsed: " << elapsed.count() << " seconds" << std::endl;

    cudaGraphExecDestroy(graphExec);
    cudaGraphDestroy(graph);

    double* result_matrix = use_matrix1 ? h_matrix1 : h_matrix2;
    cudaMemcpy(result_matrix, use_matrix1 ? d_matrix1 : d_matrix2, matrix_size, cudaMemcpyDeviceToHost);

    std::ofstream out_file("result.dat", std::ios::binary);
    out_file.write(reinterpret_cast<const char*>(result_matrix), width * width * sizeof(double));
    out_file.close();

    delete[] h_matrix1;
    delete[] h_matrix2;
    cudaFree(d_matrix1);
    cudaFree(d_matrix2);

    return 0;
}