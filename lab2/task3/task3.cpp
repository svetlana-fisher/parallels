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


void solution_for(const std::vector<double>& A, const std::vector<double>& b, double tau, double E, int N, int NUM_THREADS, int NUM_SCHEDULE){
    
    #pragma omp parallel num_threads(NUM_THREADS) 
    {
        std::vector<double> x(N);

        double E_parallel = E;

        double err = std::numeric_limits<double>::max();

        #pragma omp for schedule(static, NUM_SCHEDULE)
        for (int i = 0; i < N; i++) {
            x[i] = 0;
        }  

        double spectral_norm_b = 0; // ||b||2
        #pragma omp for schedule(static, NUM_SCHEDULE)
        for (int i=0; i<N; i++){
            double power_b = b[i]*b[i];
            #pragma omp atomic
            spectral_norm_b += power_b;
        }
        spectral_norm_b = sqrt(spectral_norm_b);

        E_parallel *= spectral_norm_b; // заменяем деление для ускорения программы

        while (err > E_parallel){
            double spectral_norm_A = 0; // ||Ax-b||2

            #pragma omp for schedule(static, NUM_SCHEDULE)
                for (int i = 0; i < N; i++) {
                    double tmp_vec = 0.0;
                    for (int j = 0; j < N; j++){
                        tmp_vec += A[i * N + j] * x[j];
                    } 
                    tmp_vec -= b[i]; // Ax-b
                    x[i] -= tau * tmp_vec;

                    tmp_vec *= tmp_vec;
                    #pragma omp atomic
                    spectral_norm_A += tmp_vec;
                }
            spectral_norm_A = sqrt(spectral_norm_A);
            err = spectral_norm_A; // убрали деление, умножив E  spectral_norm_b
        }
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

    std::vector<int> numbers = {2, 4, 6, 10, 16};
    std::vector<int> numbers_shedule = {2, 4, 6, 10, 16};

    for (int num_shedule : numbers_shedule){

        std::cout << "num_shedule=" << num_shedule << std::endl;

        auto start = std::chrono::steady_clock::now();
        solution_for(A, b, tau, E, N, 1, num_shedule);
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;
        std::cout << elapsed_seconds.count() << std::endl;


        std::vector<double> x;
        for (int num_threads : numbers) {
            start =std::chrono::steady_clock::now();
            solution_for(A, b, tau, E, N, num_threads, num_shedule);
            end = std::chrono::steady_clock::now();
            std::chrono::duration<double> other_elapsed_seconds = end - start;
            std::cout << "T" << num_threads << "=" << other_elapsed_seconds.count();
            std::cout << " S" << num_threads << "=" << elapsed_seconds.count()/other_elapsed_seconds.count() << std::endl;

        }
    }
    
    return 0;
}