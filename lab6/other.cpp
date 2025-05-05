#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <boost/program_options.hpp>
#include <chrono>

#define OUT_FILE "result.dat"

bool which_step = 0;


int NX = 20;
int NY = 20;
double EPS = 0.000001;
int ITER = 1000000;

double norm_orig = 0;


#define TAU -0.01


void init_matrix(double *matrix) {
	double corners[4] = {10.0, 20.0, 30.0, 20.0};
	
	for (int i = 0; i < (NX + 2) * (NY + 2); i++) {
		matrix[i] = 0.0;
	}
	
	matrix[(NX + 2) + 1] = corners[0];
	matrix[(NX + 2) * 2 - 2] = corners[1];
	matrix[(NX + 2) * (NY + 1) - 2] = corners[2];
	matrix[(NX + 2) * NY + 1] = corners[3];
	
	for (int i = (NX + 2) + 2, j = 1; i < (NX + 2) * 2 - 2; i++, j++) {
		double coef = (double)(j) / (NX - 1);
		matrix[i] = corners[0] * (1.0 - coef) + corners[1] * coef;
		matrix[(NX + 2) * NY + 1 + j] = corners[3] * (1.0 - coef) + corners[2] * coef;
	}

	for (int i = (NX + 2) * 2 - 2 + (NX + 2), j = 1; i < (NX + 2) * (NY + 1) - 2; i+=(NX + 2), j++) {
		double coef = (double)(j) / (NY - 1);
		matrix[i] = corners[1] * (1.0 - coef) + corners[2] * coef;
		matrix[i - NX + 1] = corners[0] * (1.0 - coef) + corners[3] * coef;
	}
	
}


void count_matrix(double* matrix_1, double* matrix_2) {
	double err = 1.0;
    int iter = 0;
	// double norm_res = 0;
	do {
        err = 0.0;
		if (which_step) {
			// из первой во вторую
      #pragma acc parallel loop collapse(2) reduction(max:err)
			for (int i = 1; i < (NY + 2) - 1; i++) {
				for (int j = 1; j < (NX + 2) - 1; j++) {
					matrix_2[i * (NX + 2) + j] = 0.25 * (matrix_1[i * (NX + 2) + j - 1] 
																				+ matrix_1[i * (NX + 2) + j + 1]
																				+ matrix_1[(i - 1) * (NX + 2) + j] 
																				+ matrix_1[(i + 1) * (NX + 2) + j]);
                err = fmax(err, fabs(matrix_2[i * NX + j] - matrix_1[i * NX + j]));
				};
			}

			// double result = 0.0;
			// #pragma acc parallel loop reduction(+:result)
			// for (int i = 0; i < (NX + 2) * (NY + 2); i++) {
			// 	result += matrix_2[i] * matrix_2[i];
			// }
			// norm_res = sqrt(result);

			which_step = 0;
		} else { 
			// из второй в первую
      #pragma acc parallel loop collapse(2)
			for (int i = 1; i < (NY + 2) - 1; i++) {
				for (int j = 1; j < (NX + 2) - 1; j++) {
					matrix_1[i * (NX + 2) + j] = 0.25 * (matrix_2[i * (NX + 2) + j - 1] 
																				+ matrix_2[i * (NX + 2) + j + 1]
																				+ matrix_2[(i - 1) * (NX + 2) + j] 
																				+ matrix_2[(i + 1) * (NX + 2) + j]);
                err = fmax(err, fabs(matrix_2[i * NX + j] - matrix_1[i * NX + j]));
				};
                
			}

			// double result = 0.0;
			// #pragma acc parallel loop reduction(+:result)
			// for (int i = 0; i < (NX + 2) * (NY + 2); i++) {
			// 	result += matrix_1[i] * matrix_1[i];
			// }
			// norm_res = sqrt(result);

			which_step = 1;
		}
		iter++;

		printf("iter: %d, %lf >= %lf\r", iter, err , EPS);
		fflush(stdout);
		
	} while (err > EPS && iter < ITER);
}


int main(int argc, char *argv[]) {
	boost::program_options::options_description desc("Options");
	desc.add_options()
		("nx", boost::program_options::value<int>(), "Matrix size in X")
		("ny", boost::program_options::value<int>(), "Matrix size in Y")
		("eps", boost::program_options::value<double>(), "Precision")
		("iters", boost::program_options::value<int>(), "Max iterations");
	
	boost::program_options::variables_map vm;

	boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
	boost::program_options::notify(vm);

	if (vm.count("nx")) NX = vm["nx"].as<int>();
	if (vm.count("ny")) NY = vm["ny"].as<int>();
	if (vm.count("eps")) EPS = vm["eps"].as<double>();
	if (vm.count("iters")) ITER = vm["iters"].as<int>();

	double *matrix_1 = new double[(NX + 2) * (NY + 2)];
	double *matrix_2 = new double[(NX + 2) * (NY + 2)];

	double result = 0;
	

	
	init_matrix(matrix_1);
	init_matrix(matrix_2);

    // for (int i = 0; i < NX+2; ++i) {
    //     for (int j = 0; j < NX+2; ++j) {
    //         std::cout << matrix_2[i * NX + j] << " ";
    //     }
    //     std::cout << std::endl;
    // }

	#pragma acc parallel loop reduction(+:result) 
	for (int i = 0; i < (NX + 2) * (NY + 2); i++) {
		result += matrix_1[i] * matrix_1[i];
	}
	norm_orig = sqrt(result);

  const auto start{std::chrono::steady_clock::now()};
	count_matrix(matrix_1, matrix_2);
	const auto end{std::chrono::steady_clock::now()};
  const std::chrono::duration<double> elapsed_seconds{end - start};
  std::cout  << std::endl << elapsed_seconds.count() << std::endl << std::endl;


  if (which_step) {
    for (int i = 1, k = 0; i < (NY + 2) - 1; i++, k++) {
      for (int j = 1, l = 0; j < (NX + 2) - 1; j++, l++) {
        matrix_2[k * NX + l] = matrix_1[i * (NX + 2) + j];
      }
    }
    FILE* f = fopen(OUT_FILE, "wb");

    fwrite(matrix_2, sizeof(double), NY * NX, f);
    fclose(f);
    // for (int i = 0; i < NY; i++) {
    // 	for (int j = 0; j < NX; j++) {
    // 		std::cout << matrix_2[i * NX + j] << ' ' ;
    // 	}
    // 	std::cout << std::endl;
    // }

  } else {
    for (int i = 1, k = 0; i < (NY + 2) - 1; i++, k++) {
      for (int j = 1, l = 0; j < (NX + 2) - 1; j++, l++) {
        matrix_1[k * NX + l] = matrix_2[i * (NX + 2) + j];
      }
    }
    FILE* f = fopen(OUT_FILE, "wb");

    fwrite(matrix_1, sizeof(double), NY * NX, f);
    fclose(f);

    // for (int i = 0; i < NY; i++) {
    // 	for (int j = 0; j < NX; j++) {
    // 		std::cout << matrix_1[i * NX + j] << ' ' ;
    // 	}
    // 	std::cout << std::endl;
    // }
  }
	

	
	
	delete[] matrix_1;
	delete[] matrix_2;
	
	return 0;
}