#include <stdio.h>
#include <iostream>
#include <vector>
#include <algorithm> 
#include <cmath>
#include <boost/program_options.hpp> // Подключение библиотеки для обработки параметров командной строки

namespace po = boost::program_options; // Создаем псевдоним для удобства

void init_matrix(double* matrix, int n) {
    matrix[0] = 10.0;
    matrix[n - 1] = 20.0;
    matrix[(n - 1) * n] = 20.0;
    matrix[(n - 1) * n + (n - 1)] = 30.0;

    for (int j = 1; j < n - 1; ++j) {
        matrix[j] = matrix[0] + (matrix[n - 1] - matrix[0]) * (j / static_cast<double>(n - 1));
        matrix[(n - 1) * n + j] = matrix[(n - 1) * n] + (matrix[(n - 1) * n + (n - 1)] - matrix[(n - 1) * n]) * (j / static_cast<double>(n - 1));
        matrix[j * n] = matrix[0] + (matrix[(n - 1) * n] - matrix[0]) * (j / static_cast<double>(n - 1));
        matrix[j * n + (n - 1)] = matrix[n - 1] + (matrix[(n - 1) * n + (n - 1)] - matrix[n - 1]) * (j / static_cast<double>(n - 1));
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

    double* matrix = new double[n * n];
    double* matrix_new = new double[n * n];

    init_matrix(matrix, n);
    init_matrix(matrix_new, n);

    double err = 1.0;
    int iter = 0;

    while (err > accuracy && iter < max_iteration) {
        err = 0.0;

        for (int i = 1; i < n - 1; i++) {
            for (int j = 1; j < n - 1; j++) {
                matrix_new[i * n + j] = 0.25 * (
                    matrix[(i + 1) * n + j] + 
                    matrix[i * n + j + 1] + 
                    matrix[i * n + j - 1] + 
                    matrix[(i - 1) * n + j]
                );
                err = fmax(err, fabs(matrix_new[i * n + j] - matrix[i * n + j]));
            }
        }

        for (int i = 1; i < n - 1; i++) {
            for (int j = 1; j < n - 1; j++) {
                matrix[i * n + j] = matrix_new[i * n + j];
            }
        }
        iter++;
    }

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