#ifndef RHYTHMIC_DELIVERY_H
#define RHYTHMIC_DELIVERY_H


#include "aux_module.h"

struct RhythmicResult {
    Vec x;
    Vec V;
    double Mp;
    bool ok;
};


void clamp_vec(Vec& vec, Vec const& lb, Vec const& ub);


RhythmicResult solve_rhythmic_delivery(Vec const& p, double V0, double minV, double maxV);


#endif