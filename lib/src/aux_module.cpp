#include "aux_module.h"



Vec operator-(Vec const& vec1, Vec const& vec2) {

    int vec_sz = vec1.size();
    Vec res(vec_sz, 0.0);
    for(int i = 0; i < vec_sz; ++i) {
        res[i] = vec1[i] - vec2[i];
    }
    return res;

}


Vec operator*(Vec const& vec, double num) {

    int vec_sz = vec.size();
    Vec res(vec_sz, 0.0);
    for(int i = 0; i < vec_sz; ++i) {
        res[i] = vec[i] * num;
    }
    return res;

}


Vec operator*(double num, Vec const& vec) {
    return vec * num;
}


double lc_norm(Vec const& vec) {

    double norm = 0.0;
    int vec_sz = vec.size();
    for(int i = 0; i < vec_sz; ++i) {
        norm = std::max(norm, std::abs(vec[i]));
    }
    return norm;

}