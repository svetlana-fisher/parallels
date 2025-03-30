#include <stdio.h>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

void init(std::vector<int>& matrix, std::vector<int>& vector, std::vector<int>& final_vector, int begin, int end, int n){
    
    for (int i=begin; i<end; i++){
        for (int j=0; j<n; j++){
            // if (i==j){
            //     matrix[i*n + j] = 1;
            // }
            // else{
            //     matrix[i*n + j] = 0;
            // }
            matrix[i * n + j] = i + j;
        }
        vector[i] = 2;
        final_vector[i] = 0;
    }
}


void mul_mtx_vec(const std::vector<int>& matrix, const std::vector<int>& vector, std::vector<int>& final_vector, int begin, int end, int n){
    for (int i=begin; i<end; i++){
        for (int j=0; j<n; j++){
            final_vector[i] += matrix[i * n + j] * vector[j];
        }
    }
}


int main(int argc, char* argv[]){
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <n>" << std::endl;
        return 1;
    }
    int n = std::atoi(argv[1]);

    std::vector<int> matrix(n*n);
    std::vector<int> vector(n);
    std::vector<int> final_vector(n);

    const auto start_t{std::chrono::steady_clock::now()};

    unsigned int num_threads = std::thread::hardware_concurrency();
    int begin;
    int end;
    std::vector<std::thread> threads;
    std::vector<std::thread> init_threads;
    int items_per_thread = n / num_threads;

    for (int id=0; id<num_threads; id++){
        begin = id * items_per_thread;
        end = (id == num_threads-1) ? (n) : (items_per_thread+begin);
        init_threads.emplace_back(init, std::ref(matrix), std::ref(vector),
             std::ref(final_vector), begin, end, n);

    }

    for (int i=0; i<num_threads; i++){
        init_threads[i].join();
    }

    for (int id=0; id<num_threads; id++){
        begin = id * items_per_thread;
        end = (id == num_threads-1) ? (n) : (items_per_thread+begin);
        threads.emplace_back(mul_mtx_vec, std::ref(matrix), std::ref(vector), std::ref(final_vector), begin, end, n);
    }
    for (int i=0; i<num_threads; i++){
        threads[i].join();
    }

    // for (int i=0; i<n; i++){
    //     if (final_vector[i] !=2){
    //         std::cout<<"ERROR"<<std::endl;
    //     }
    //     // std::cout<<final_vector[i]<<std::endl;
    // }
    const auto end_t{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{end_t - start_t};
    std::cout << elapsed_seconds.count() << std::endl;

    return 0;
}