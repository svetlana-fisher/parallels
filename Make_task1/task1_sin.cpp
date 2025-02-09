#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>

#define ARR_SIZE 10000000

template <typename T>
T sin_sum(){
    std::vector<T> sin_arr;
    T x;
    T summ = 0;

    for (int i=0; i<ARR_SIZE; i++){
        x = (2 * M_PI / ARR_SIZE) * i;
        sin_arr.push_back(sin(x));
        summ += sin_arr[i];
    }
    return summ;
}


int main(int argc, char **argv){
    if (argc < 2){
        std::cerr<<"incorrect type, please use 'float' or 'double'"<<std::endl;
        return 1;
    }

    if (strcmp(argv[1], "float") == 0){
        std::cout<<sin_sum<float>()<<std::endl;
    }
    else if (strcmp( argv[1], "double") == 0){
        std::cout<<sin_sum<double>()<<std::endl;
    }
    else{
        std::cerr<<"incorrect type, please use 'float' or 'double'"<<std::endl;
    }
    return 0;
}
