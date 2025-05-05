#include <stdio.h>
#include <iostream>
#include <vector>
#include <algorithm> 
#include <cmath>
#include <fstream>
#include <chrono>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
double norm_orig = 0;
int n = 20;
double accuracy = 1e-6;
int max_iteration = 1000000;
bool which_step = 0;


// void init_matrix(double* matrix) {
//     matrix[n+1] = 10.0;
//     matrix[n - 1 + n - 1] = 20.0;
//     matrix[(n - 1) * (n - 1)] = 20.0;
//     matrix[(n - 1) * (n - 1) + (n - 1) - 2] = 30.0;

//     for (int j = 2; j < n - 2; j++) {
//         matrix[n + j] = matrix[n + 1] + (matrix[n - 1 + n - 1] - matrix[n + 1]) * (static_cast<double>(j-1) / static_cast<double>(n - 3));
//         matrix[(n - 1) * (n - 1) + j - 1] = matrix[(n - 1) * (n - 1)] + (matrix[(n - 1) * (n - 1) + (n - 1) - 2] - matrix[(n - 1) * (n - 1)]) * (static_cast<double>(j-1) / static_cast<double>(n - 3));
//         matrix[j * n + 1] = matrix[n + 1] + (matrix[(n - 1) * (n - 1)] - matrix[n + 1]) * (static_cast<double>(j-1) / static_cast<double>(n - 3));
//         matrix[j * n + (n - 1) - 1] = matrix[n - 1 + n - 1] + (matrix[(n - 1) * (n - 1) + (n - 1) - 2] - matrix[n - 1 + n - 1]) * (static_cast<double>(j-1) / static_cast<double>(n - 3));
//     }
// }

void init_matrix(double *matrix) {
	double corners[4] = {10.0, 20.0, 30.0, 20.0};
	
	for (int i = 0; i < (n + 2) * (n + 2); i++) {
		matrix[i] = 0.0;
	}
	
	matrix[(n + 2) + 1] = corners[0];
	matrix[(n + 2) * 2 - 2] = corners[1];
	matrix[(n + 2) * (n + 1) - 2] = corners[2];
	matrix[(n + 2) * n + 1] = corners[3];
	
	for (int i = (n + 2) + 2, j = 1; i < (n + 2) * 2 - 2; i++, j++) {
		double coef = (double)(j) / (n - 1);
		matrix[i] = corners[0] * (1.0 - coef) + corners[1] * coef;
		matrix[(n + 2) * n + 1 + j] = corners[3] * (1.0 - coef) + corners[2] * coef;
	}

	for (int i = (n + 2) * 2 - 2 + (n + 2), j = 1; i < (n + 2) * (n + 1) - 2; i+=(n + 2), j++) {
		double coef = (double)(j) / (n - 1);
		matrix[i] = corners[1] * (1.0 - coef) + corners[2] * coef;
		matrix[i - n + 1] = corners[0] * (1.0 - coef) + corners[3] * coef;
	}
	
}


void count_matrix(double* matrix, double* matrix_new){
    double err = 1.0;
    int iter = 0;
    while (err > accuracy && iter < max_iteration) {
        err = 0.0;

        #pragma acc parallel loop collapse(2) reduction(max:err)
        for (int i = 1; i < (n + 2) - 1; i++) {
            for (int j = 1; j < (n + 2) - 1; j++) {
                matrix_new[i * (n + 2) + j] = 0.25 * (
                matrix[i * (n + 2) + j - 1] 
                + matrix[i * (n + 2) + j + 1]
                + matrix[(i - 1) * (n + 2) + j] 
                + matrix[(i + 1) * (n + 2) + j]);
                err = fmax(err, fabs(matrix_new[i * (n + 2) + j] - matrix[i * (n + 2) + j]));
            }
        }

        #pragma acc parallel loop
        for (int i = 1; i < (n + 2) - 1; i++) {
            for (int j = 1; j < (n + 2) - 1; j++) {
                matrix[i * (n + 2) + j] = matrix_new[i * (n + 2) + j];
            }
        }
        iter++;
    }
}

int main(int argc, char** argv) {

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

    double* matrix = new double[(n + 2) * (n + 2)];
    double* matrix_new = new double[(n + 2) * (n + 2)];

    init_matrix(matrix);
    init_matrix(matrix_new);

    const auto start{std::chrono::steady_clock::now()};
    count_matrix(matrix, matrix_new);
    const auto end{std::chrono::steady_clock::now()};
    const std::chrono::duration<double> elapsed_seconds{end - start};
    std::cout  << elapsed_seconds.count() << std::endl;

    // Вывод матрицы
    // for (int i = 0; i < n; ++i) {
    //     for (int j = 0; j < n; ++j) {
    //         std::cout << matrix[i * n + j] << " ";
    //     }
    //     std::cout << std::endl;
    // }

    std::ofstream out_file("result.dat", std::ios::binary);
    out_file.write(reinterpret_cast<const char*>(matrix), n * n * sizeof(double));
    out_file.close();

    // std::cout << "Iterations: " << iter << "\n";
    // std::cout << "Final error: " << err << "\n";

    delete[] matrix;
    delete[] matrix_new;
    return 0;
}