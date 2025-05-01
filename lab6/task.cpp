#include <stdio.h>
#include <iostream>
#include <vector>
#include <algorithm> 
#include <cmath>

#define MAX_ITERATION 1000000
#define ACCURACY 0.0000001

void init_matrix(std::vector<double>& matrix, int n) {
    matrix[0] = 10.0;
    matrix[n - 1] = 20.0;
    matrix[(n - 1) * n] = 20.0;
    matrix[(n - 1) * n + (n - 1)] = 30.0;

    for (int j = 1; j < n - 1; ++j) {
        matrix[j] = matrix[0] + (matrix[n - 1] - matrix[0]) * (j / static_cast<double>(n - 1));
        matrix[(n - 1) * n + j] = matrix[(n - 1) * n] + (matrix[(n - 1) * n + (n - 1)] - matrix[(n - 1) * n]) * (j / static_cast<double>(n - 1));
        matrix[j * n] = matrix[0] + (matrix[(n - 1) * n] - matrix[0]) * (j / static_cast<double>(n - 1));
        matrix[j * n + (n - 1)] = matrix[(n - 1)] + (matrix[(n - 1) * n + (n - 1)] - matrix[(n - 1) * n]) * (j / static_cast<double>(n - 1));
    }
}

int main() {
    int n = 20;
    std::vector<double> matrix(n * n, 0.0);
    std::vector<double> matrix_new(n * n, 0.0);

    init_matrix(matrix, n);
    init_matrix(matrix_new, n);

    double err = 1.0;
    int iter = 0;

    // Копируем данные на устройство
    #pragma acc enter data copyin(matrix[0:n*n], matrix_new[0:n*n], err, iter, n)

    while (err > ACCURACY && iter < MAX_ITERATION) {
        err = 0.0;
        
        // Вычисляем новые значения и ошибку
        #pragma acc parallel loop present(matrix, matrix_new) independent collapse(2) reduction(max:err)
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

        // Копируем новые значения обратно в matrix
        #pragma acc parallel loop present(matrix, matrix_new) independent collapse(2)
        for (int i = 1; i < n - 1; i++) {
            for (int j = 1; j < n - 1; j++) {
                matrix[i * n + j] = matrix_new[i * n + j];
            }
        }
        
        iter++;
    }

    // Копируем данные обратно на хост
    #pragma acc exit data copyout(matrix[0:n*n])

    // Вывод матрицы
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            std::cout << matrix[i * n + j] << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}