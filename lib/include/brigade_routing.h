#ifndef BRIGADE_ROUTING_H
#define BRIGADE_ROUTING_H

#include "aux_module.h"

#include <string>
#include <vector>

// ============================================================
// Public API (inputs)
// ============================================================

// Undirected road edge between cities (distance in km).
struct RoadEdge {
    std::string from;
    std::string to;
    double km = 0.0;
};

// One worker.
struct WorkerInput {
    std::string name;
    std::string city;
    Veci skills; // operation types they can perform
};

// One request (job).
struct RequestInput {
    int id = -1;
    std::string city;
    Veci tasks; // required operation types (NO repeats assumed)
};

struct Params {
    double speed_kmph = 60.0; // shared routing speed
    double p1 = 1.0, p2 = 1.0, p3 = 1.0, p4 = 1.0;
};

// ============================================================
// Public API (outputs)
// ============================================================

struct RequestSolution {
    int requestId = -1;
    bool feasible = true;

    Veci team; // indices of workers in the input workers vector

    // assignment[workerIndex] = list of operation types assigned to that worker
    // size == workers.size()
    VecVeci assignment;

    double lc = INF;

    double startTime = 0.0;
    double finishTime = 0.0;

    double distKm = INF;
    double t2Hours = INF;
};

struct Solution {
    std::vector<RequestSolution> perRequest; // same order as input requests
};

// ============================================================
// Solver
// ============================================================

// Solve sequentially in the provided order (indices into requests).
// If order is empty, uses natural order 0..M-1.
// w can be provided; if empty, it will be computed as demand/supply.
Solution solve_with_roads(
    std::vector<RequestInput> const& requests,
    std::vector<WorkerInput>  const& workers,
    Vecr const& tau,                 // tau[op]
    std::vector<RoadEdge> const& roads,
    Params const& P,
    Veci order = {},
    bool dynamicW = false
);

#endif
