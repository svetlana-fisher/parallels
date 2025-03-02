// Привязать program к cpu 1,2,3: taskset -c 1,2,3 program
// Привязать program к ноде 0: numactl -N 0 program
// Привязать program к cpu 4,5,6: numactl -C 4,5,6 program

// taskset -c 1-10 ./task1 20000


#include <stdio.h>
#include <cstdlib>
#include <omp.h>
#include <chrono>
#include <iostream>
#include <cstdlib>

void run_parallel(int m, int n)
{
    double *a, *b, *c;

    a = new double[m * n];
    b = new double[n];
    c = new double[m];


    // Подготовка данных для расчетов
    #pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int items_per_thread = m / nthreads;
        int lb = threadid * items_per_thread;
        int ub = (threadid == nthreads - 1) ? (m - 1) : (lb + items_per_thread - 1);
        for (int i = lb; i <= ub; i++) {
            for (int j = 0; j < n; j++){
                a[i * n + j] = i + j;
            }    
            c[i] = 0.0;
        }
    }
    for (int j = 0; j < n; j++){
        b[j] = j;
    }


    #pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int items_per_thread = m / nthreads;
        int lb = threadid * items_per_thread;
        int ub = (threadid == nthreads - 1) ? (m - 1) : (lb + items_per_thread - 1);
        
        for (int i = lb; i <= ub; i++) {
            for (int j = 0; j < n; j++){
                c[i] += a[i * n + j] * b[j];
            }
        }
    }

    delete[] a;
    delete[] b;
    delete[] c;
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <m>" << std::endl;
        return 1;
    }

    // Преобразуем аргументы командной строки в целые числа
    int m = std::atoi(argv[1]);

    const auto start{std::chrono::steady_clock::now()};
    run_parallel(m, m);
    const auto end{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{end - start};

    std::cout << elapsed_seconds.count() << std::endl;
    
    return 0;
}