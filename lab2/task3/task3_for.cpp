#include <stdio.h>
#include <cstdlib>
#include <omp.h>
#include <chrono>
#include <iostream>
#include <cstdlib> // Для std::atoi
#include <vector>
#include <cmath>
#include <limits>


std::vector<double> init_A_parallel_for(int N, int NUM_THREADS){
    std::vector<double> A(N * N);

    #pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++){
            A[i * N + j] = 1.0;
        } 
        //A[i*N+j] i=j   
        A[i*(N+1)] = 2.0;
    }  

    return std::move(A);
}

std::vector<double> init_B_parallel_for(int N, int NUM_THREADS){
    std::vector<double> B(N);

    double N1 = N + 1;

    #pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < N; i++) {
        B[i] = N1;
    }  
    return std::move(B);
}

std::vector<double> init_x_parallel_for(int N, int NUM_THREADS){
    std::vector<double> x(N);

    #pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < N; i++) {
        x[i] = 0;
    }  
    return std::move(x);
}

void solution_for(const std::vector<double>& A, std::vector<double>& x, const std::vector<double>& b, double tau, double E, int N, int NUM_THREADS){
    std::vector<double> tmp_vec(N);

    double err = std::numeric_limits<double>::max();

    double spectral_norm_b = 0; // ||b||2
    #pragma omp parallel for num_threads(NUM_THREADS)
    for (int i=0; i<N; i++){
        double power_b = b[i]*b[i];
        #pragma omp atomic
        spectral_norm_b += power_b;
    }
    spectral_norm_b = sqrt(spectral_norm_b);


    while (err > E){
        double spectral_norm_A = 0; // ||Ax-b||2

        #pragma omp parallel for num_threads(NUM_THREADS)
            for (int i = 0; i < N; i++) {
                tmp_vec[i] = 0.0;
                for (int j = 0; j < N; j++){
                    tmp_vec[i] += A[i * N + j] * x[j];
                } 
                tmp_vec[i] -= b[i]; // Ax-b
                x[i] -= tau * tmp_vec[i];

                tmp_vec[i] *= tmp_vec[i];
                #pragma omp atomic
                spectral_norm_A += tmp_vec[i];
            }
        spectral_norm_A = sqrt(spectral_norm_A);
        err = spectral_norm_A / spectral_norm_b;
    }

}



int main(int argc, char* argv[]) {

    double E = 0.0000001;

    double tau = 0.001;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <N>" << std::endl;
        return 1;
    }

    // Преобразуем аргументы командной строки в целые числа
    int N = std::atoi(argv[1]);

    // Заполняем A
    std::vector<double> A = init_A_parallel_for(N, 4);
    // Заполняем B
    std::vector<double> b = init_B_parallel_for(N, 4);


    // заполняем x
    std::vector<double> x = init_x_parallel_for(N, 4);
    auto start = std::chrono::steady_clock::now();
    solution_for(A, x, b, tau, E, N, 1);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << elapsed_seconds.count() << std::endl;


    std::vector<int> numbers = {2, 4, 6, 10, 16};
    for (int num_threads : numbers) {
        // заполняем x
        x = init_x_parallel_for(N, 4);
        start =std::chrono::steady_clock::now();
        solution_for(A, x, b, tau, E, N, num_threads);
        end = std::chrono::steady_clock::now();
        std::chrono::duration<double> other_elapsed_seconds = end - start;
        std::cout << "T" << num_threads << "=" << other_elapsed_seconds.count();
        std::cout << " S" << num_threads << "=" << elapsed_seconds.count()/other_elapsed_seconds.count() << std::endl;
    }
    
    return 0;
}