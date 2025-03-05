// g++ -fopenmp task3.cpp -o task3

#include <stdio.h>
#include <cstdlib>
#include <omp.h>
#include <chrono>
#include <iostream>
#include <cstdlib> // Для std::atoi
#include <vector>
#include <cmath>
#include <limits>

std::vector<double> init_A_parallel(int N){
    std::vector<double> A(N * N);

    #pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int items_per_thread = N / nthreads;
        int lb = threadid * items_per_thread;
        int ub = (threadid == nthreads - 1) ? (N - 1) : (lb + items_per_thread - 1);
        for (int i = lb; i <= ub; i++) {
            for (int j = 0; j < N; j++){
                A[i * N + j] = 1.0;
            } 
            //A[i*N+j] i=j   
            A[i*(N+1)] = 2.0;
        }  
    }
    return std::move(A);
}


std::vector<double> init_B_parallel(int N){
    std::vector<double> B(N);

    double N1 = N + 1;

    #pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int items_per_thread = N / nthreads;
        int lb = threadid * items_per_thread;
        int ub = (threadid == nthreads - 1) ? (N - 1) : (lb + items_per_thread - 1);
        for (int i = lb; i <= ub; i++) {
            B[i] = N1;
        }  
    }
    return std::move(B);
}

std::vector<double> init_x_parallel(int N){
    std::vector<double> x(N);
    #pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int items_per_thread = N / nthreads;
        int lb = threadid * items_per_thread;
        int ub = (threadid == nthreads - 1) ? (N - 1) : (lb + items_per_thread - 1);
        for (int i = lb; i <= ub; i++) {
            x[i] = 0;
        }  
    }
    return std::move(x);
}

void solution(const std::vector<double>& A, std::vector<double>& x, const std::vector<double>& b, double tau, double E, int N, int NUM_THREADS){
    #pragma omp parallel num_threads(NUM_THREADS)
    {
    std::vector<double> tmp_vec(N);

    double err = std::numeric_limits<double>::max();

    double spectral_norm_b = 0; // ||b||2
    for (int i=0; i<N; i++){
        spectral_norm_b += b[i]*b[i];
    }
    spectral_norm_b = sqrt(spectral_norm_b);


    while (err > E){
        double spectral_norm_A = 0; // ||Ax-b||2

        // #pragma omp parallel num_threads(NUM_THREADS)
        // {
            int nthreads = omp_get_num_threads();
            int threadid = omp_get_thread_num();
            int items_per_thread = N / nthreads;
            int lb = threadid * items_per_thread;
            int ub = (threadid == nthreads - 1) ? (N - 1) : (lb + items_per_thread - 1);

            double part_spectral_norm_A = 0;

            for (int i = lb; i <= ub; i++) {
                tmp_vec[i] = 0.0;
                for (int j = 0; j < N; j++){
                    tmp_vec[i] += A[i * N + j] * x[j];
                } 
                tmp_vec[i] -= b[i]; // Ax-b
                part_spectral_norm_A += tmp_vec[i] * tmp_vec[i];
                x[i] -= tau * tmp_vec[i];
            // }
            #pragma omp atomic
            spectral_norm_A += part_spectral_norm_A;
        }
        spectral_norm_A = sqrt(spectral_norm_A);
        err = spectral_norm_A / spectral_norm_b;
    }
    }

}



int main(int argc, char* argv[]) {

    double E = 0.0000001;

    double tau = 0.001;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <N>" << std::endl;
        return 1; // Возвращаем код ошибки
    }

    // Преобразуем аргументы командной строки в целые числа
    int N = std::atoi(argv[1]);

    // Заполняем A
    std::vector<double> A = init_A_parallel(N);
    // Заполняем B
    std::vector<double> b = init_B_parallel(N);


    // заполняем x
    std::vector<double> x = init_x_parallel(N);
    auto start = std::chrono::steady_clock::now();
    solution(A, x, b, tau, E, N, 1);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "T1=" << elapsed_seconds.count() << std::endl;


    std::vector<int> numbers = {2, 4, 6, 10, 16};
    for (int num_threads : numbers) {
        // заполняем x
        x = init_x_parallel(N);
        start =std::chrono::steady_clock::now();
        solution(A, x, b, tau, E, N, num_threads);
        end = std::chrono::steady_clock::now();
        std::chrono::duration<double> other_elapsed_seconds = end - start;
        std::cout << "T" << num_threads << "=" << other_elapsed_seconds.count();
        std::cout << " S" << num_threads << "=" << elapsed_seconds.count()/other_elapsed_seconds.count() << std::endl;
    }
    
    return 0;
}