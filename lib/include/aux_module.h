#ifndef AUX_MODULE_H
#define AUX_MODULE_H


#include <cmath>
#include <vector>
#include <utility>
#include <random>
#include <algorithm>
#include <numeric>
#include <functional>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <limits>
#include <string>


inline constexpr double INF = std::numeric_limits<double>::infinity();


const double pi = std::acos(-1.0); // число Pi

using Vecr = std::vector<double>; // синоним для вещественного вектора
using Matrix = std::vector<Vecr>; // синоним для вещественной матрицы



Vecr operator-(Vecr const& Vec1, Vecr const& Vec2); // оператор вычитания векторов


Vecr operator*(Vecr const& Vec, double num); // оператор умножения вектора на число


Vecr operator*(double num, Vecr const& Vec); // оператор умножения вектора на число


double lc_norm(Vecr const& Vec); // l_inf норма


using Vecc = std::vector<char>;
using Veci = std::vector<int>;
using VecVeci = std::vector<Veci>;
using Pairii = std::pair<int, int>;
using VecPairii = std::vector<Pairii>;
using VecVecPairii = std::vector<VecPairii>;



#endif