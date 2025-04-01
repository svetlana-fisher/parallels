#include <iostream>
#include <fstream>
#include <cmath>
#include <string>

int main() {
    std::ifstream input_sin("./client_sin.txt");
    std::string line;

    while (std::getline(input_sin, line)) {
        double angle = std::stod(line.substr(4, line.find(')') - 4));
        double expectedSin = std::stod(line.substr(line.find('=') + 1));
        double calculatedSin = std::sin(angle);
        if (abs(calculatedSin - expectedSin) > 0.0001){
            std::cout << "ERROR!!!"<<std::endl;
            std::cout << "sin(" << angle << ") = " << calculatedSin << " (ожидаемое: " << expectedSin << ")"<<std::endl;
            input_sin.close();
            return 1;
        }
    }

    input_sin.close();


    std::ifstream input_sqrt("./client_sqrt.txt");
    // std::string line;

    while (std::getline(input_sqrt, line)) {
        double value = std::stod(line.substr(5, line.find(')') - 5));
        double expectedSqrt = std::stod(line.substr(line.find('=') + 1));
        double calculatedSqrt = std::sqrt(value);
        
        if (std::abs(calculatedSqrt - expectedSqrt) > 0.0001) {
            std::cout << "ERROR!!!" << std::endl;
            std::cout << "sqrt(" << value << ") = " << calculatedSqrt << " (ожидаемое: " << expectedSqrt << ")" << std::endl;
            input_sqrt.close();
            return 1;
        }
    }

    input_sqrt.close();


    std::ifstream input_pow("./client_pow.txt");
    // std::string lise;

    while (std::getline(input_pow, line)) {
        double base = std::stod(line.substr(0, line.find('^')));
        double exponent = std::stod(line.substr(line.find('^') + 1, line.find('=') - line.find('^') - 1));
        double expectedResult = std::stod(line.substr(line.find('=') + 1));
        double calculatedResult = std::pow(base, exponent);
        
        if (std::abs(calculatedResult - expectedResult) > 1) {
            std::cout << "ERROR!!!" << std::endl;
            std::cout << base << "^" << exponent << " = " << calculatedResult << " (ожидаемое: " << expectedResult << ")" << std::endl;
            input_pow.close();
            return 1;
        }
    }

    input_pow.close();
    std::cout << "Yapi!! it's done, well done :)" << std::endl;
    return 0;
}

