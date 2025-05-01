#include <stdio.h>
#include <iostream>
#include <vector>
#include <algorithm> 
#include <cmath>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

void init_matrix(double* matrix, int n) {
    // Устанавливаем угловые значения
    matrix[0] = 10.0;                      // верхний левый
    matrix[n-1] = 20.0;                    // верхний правый
    matrix[(n-1)*n] = 20.0;                // нижний левый
    matrix[(n-1)*n + (n-1)] = 30.0;        // нижний правый

    // Линейная интерполяция границ
    #pragma acc parallel loop present(matrix)
    for (int j = 1; j < n-1; ++j) {
        // Верхняя граница
        matrix[j] = matrix[0] + (matrix[n-1] - matrix[0]) * (j / static_cast<double>(n-1));
        // Нижняя граница
        matrix[(n-1)*n + j] = matrix[(n-1)*n] + 
                              (matrix[(n-1)*n + (n-1)] - matrix[(n-1)*n]) * 
                              (j / static_cast<double>(n-1));
        // Левая граница
        matrix[j*n] = matrix[0] + (matrix[(n-1)*n] - matrix[0]) * 
                      (j / static_cast<double>(n-1));
        // Правая граница
        matrix[j*n + (n-1)] = matrix[n-1] + 
                              (matrix[(n-1)*n + (n-1)] - matrix[n-1]) * 
                              (j / static_cast<double>(n-1));
    }

    // Заполняем внутреннюю область нулями
    #pragma acc parallel loop collapse(2) present(matrix)
    for (int i = 1; i < n-1; ++i) {
        for (int j = 1; j < n-1; ++j) {
            matrix[i*n + j] = 0.0;
        }
    }
}

int main(int argc, char** argv) {
    // Параметры по умолчанию
    int n = 20;
    double accuracy = 1e-6;
    int max_iteration = 1000000;

    // Настройка обработки параметров командной строки
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

    // Проверка допустимых размеров сетки
    // if (n != 128 && n != 256 && n != 512 && n != 1024) {
    //     std::cerr << "Error: Invalid grid size. Allowed sizes are 128, 256, 512, 1024.\n";
    //     return 1;
    // }

    // Выделяем память с учетом выравнивания для GPU
    double* matrix = new double[n*n];
    double* matrix_new = new double[n*n];

    // Инициализация данных на GPU
    #pragma acc enter data create(matrix[0:n*n], matrix_new[0:n*n])
    
    init_matrix(matrix, n);
    init_matrix(matrix_new, n);

    double err = 1.0;
    int iter = 0;

    // Основной цикл решения
    while (err > accuracy && iter < max_iteration) {
        err = 0.0;

        // Вычисление новых значений
        #pragma acc parallel loop collapse(2) present(matrix, matrix_new) reduction(max:err)
        for (int i = 1; i < n-1; i++) {
            for (int j = 1; j < n-1; j++) {
                matrix_new[i*n + j] = 0.25 * (
                    matrix[(i+1)*n + j] + 
                    matrix[(i-1)*n + j] + 
                    matrix[i*n + (j+1)] + 
                    matrix[i*n + (j-1)]
                );
                err = fmax(err, fabs(matrix_new[i*n + j] - matrix[i*n + j]));
            }
        }

        // Обновление матрицы
        #pragma acc parallel loop collapse(2) present(matrix, matrix_new)
        for (int i = 1; i < n-1; i++) {
            for (int j = 1; j < n-1; j++) {
                matrix[i*n + j] = matrix_new[i*n + j];
            }
        }

        iter++;
        
        // Периодическая проверка ошибки на CPU (каждые 100 итераций)
        if (iter % 100 == 0) {
            #pragma acc update self(err)
            if (err <= accuracy) break;
        }
    }

    // Финализация
    #pragma acc exit data delete(matrix[0:n*n], matrix_new[0:n*n])

    // Вывод матрицы
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            std::cout << matrix[i * n + j] << " ";
        }
        std::cout << std::endl;
    }
    
    std::cout << "Iterations: " << iter << "\n";
    std::cout << "Final error: " << err << "\n";

    delete[] matrix;
    delete[] matrix_new;
    return 0;
}