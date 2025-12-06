#ifndef AUX_MODULE_H
#define AUX_MODULE_H


#include <algorithm>
#include <cmath>
#include <vector>


using Vec = std::vector<double>; // синоним для вещественного вектора
using Matrix = std::vector<Vec>; // синоним для вещественной матрицы


Vec operator-(Vec const& vec1, Vec const& vec2); // оператор вычитания векторов


Vec operator*(Vec const& vec, double num); // оператор умножения вектора на число


Vec operator*(double num, Vec const& vec); // оператор умножения вектора на число


double lc_norm(Vec const& vec); // l_inf норма


#endif