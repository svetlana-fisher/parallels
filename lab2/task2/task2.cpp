// g++ -fopenmp ./laba0_task2.cpp -o laba0_task2

#include <stdio.h>
#include <iostream>
#include <omp.h>
#include <chrono>
#include <math.h>


double func(double x)
{
    return exp(-x * x);
}

double integrate_omp(double (*func)(double), double a, double b, int n)
{
    double h = (b - a) / n;
    double sum = 0.0;

    #pragma omp parallel
    {
        int nthreads = omp_get_num_threads();
        int threadid = omp_get_thread_num();
        int items_per_thread = n / nthreads;
        int lb = threadid * items_per_thread;
        int ub = (threadid == nthreads - 1) ? (n - 1) : (lb + items_per_thread - 1);
        double sumloc = 0.0;

        for (int i = lb; i <= ub; i++){
            sumloc += func(a + h * (i + 0.5));
        }
    
        #pragma omp atomic
        sum += sumloc;
    }
    
    sum *= h;
    return sum;
}


int main(){
    const double a = -4.0;
    const double b = 4.0;
    const int nsteps = 40000000;

    const auto start{std::chrono::steady_clock::now()};
    integrate_omp(func, a, b, nsteps);
    const auto end{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{end - start};

    // std::cout << "Your calculations took " <<
    //             elapsed_seconds.count() <<
    //             " seconds to run chrono.\n"; // Before C++20

    std::cout << elapsed_seconds.count() << std::endl;

    return 0;
}