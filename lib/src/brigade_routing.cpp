#include "brigade_routing.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <queue>
#include <stdexcept>
#include <unordered_map>

// ============================================================
// Internal model
// ============================================================

struct CityGraph {
    std::vector<std::vector<std::pair<int,double>>> adj; // (to, km)
};

static inline void add_undirected_edge(CityGraph& g, int u, int v, double km) {
    if (u < 0 || v < 0) return;
    if (u >= (int)g.adj.size() || v >= (int)g.adj.size()) return;
    g.adj[u].push_back({v, km});
    g.adj[v].push_back({u, km});
}

static std::vector<double> dijkstra_km(int src, CityGraph const& g) {
    int n = (int)g.adj.size();
    std::vector<double> dist(n, INF);
    dist[src] = 0.0;

    using Node = std::pair<double,int>; // (dist, v)
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq;
    pq.push({0.0, src});

    while (!pq.empty()) {
        auto [d, v] = pq.top();
        pq.pop();
        if (d != dist[v]) continue; // stale
        for (auto const& [to, w] : g.adj[v]) {
            if (dist[to] > d + w) {
                dist[to] = d + w;
                pq.push({dist[to], to});
            }
        }
    }

    return dist;
}

struct DistCache {
    CityGraph g;
    std::vector<std::vector<double>> cache; // cache[src] = dist[]
    std::vector<char> ready;               // ready[src] = 1 if computed

    explicit DistCache(CityGraph&& graph) : g(std::move(graph)) {
        int n = (int)g.adj.size();
        cache.resize(n);
        ready.assign(n, 0);
    }

    double get_km(int src, int dst) {
        if (src < 0 || dst < 0) return INF;
        if (src >= (int)g.adj.size() || dst >= (int)g.adj.size()) return INF;
        if (!ready[src]) {
            cache[src] = dijkstra_km(src, g);
            ready[src] = 1;
        }
        return cache[src][dst];
    }
};

struct Worker {
    std::string name;
    int cityId = -1;
    Veci skills;
    Vecc can; // can[op]
};

struct Request {
    int id = -1;
    int cityId = -1;
    Veci tasks; // required ops (unique)
};

// ============================================================
// Utilities: compute w, coverage
// ============================================================

static Vecr compute_w(std::vector<Request> const& requests, std::vector<Worker> const& workers, int N) {
    Veci demand(N, 0), supply(N, 0);
    for (auto const& r : requests) {
        for (int op : r.tasks) demand[op]++;
    }
    for (auto const& w : workers) {
        for (int op : w.skills) supply[op]++;
    }

    Vecr out(N, 0.0);
    for (int i = 0; i < N; ++i) {
        if (demand[i] > 0 && supply[i] == 0) {
            throw std::runtime_error("No worker has required operation type " + std::to_string(i));
        }
        if (supply[i] == 0) out[i] = 0.0;
        else out[i] = (double)demand[i] / (double)supply[i];
    }
    return out;
}

static bool covers_all(Request const& r, Veci const& team, std::vector<Worker> const& workers) {
    for (int op : r.tasks) {
        bool ok = false;
        for (int j : team) {
            if (workers[j].can[op]) { ok = true; break; }
        }
        if (!ok) return false;
    }
    return true;
}


// ============================================================
// LC for a fixed assignment (assign[tpos] = workerIndex)
// ============================================================

static double compute_LC_for_fixed_assignment(
    Request const& r,
    Veci const& team,
    std::vector<int> const& assign, // size R: worker index for each task position
    std::vector<Worker> const& workers,
    Vecr const& tau,
    Vecr const& w,
    Vecr const& avail,
    Params const& P,
    double t2_hours,
    double& outTop
) {
    if (team.empty()) return INF;
    const int R = (int)r.tasks.size();
    const int K = (int)workers.size();

    if ((int)assign.size() != R) return INF;
    if ((int)team.size() > R) return INF;

    // start time for this team
    double start = 0.0;
    for (int j : team) start = std::max(start, avail[j]);

    // p3_term = sum(avail[j] * |skills(j)|)
    double p3_term = 0.0;
    for (int j : team) p3_term += avail[j] * (double)workers[j].skills.size();

    // travel term: 2*t2 * R
    if (!(t2_hours < INF)) return INF;
    double travel_term = 2.0 * t2_hours * (double)R;

    // fast team membership check
    std::vector<char> inTeam(K, 0);
    for (int j : team) inTeam[j] = 1;

    // skillsSumW[j]
    std::vector<double> skillsSumW(K, 0.0);
    double sumSkillsSumW = 0.0;
    for (int j : team) {
        double s = 0.0;
        for (int skill : workers[j].skills) s += w[skill];
        skillsSumW[j] = s;
        sumSkillsSumW += s;
    }

    // accumulators
    std::vector<double> load(K, 0.0);
    std::vector<double> wsum(K, 0.0);
    std::vector<double> usedSumW(K, 0.0);
    std::vector<int> cnt(K, 0);

    for (int tpos = 0; tpos < R; ++tpos) {
        int op = r.tasks[tpos];
        int j  = assign[tpos];

        if (j < 0 || j >= K) return INF;
        if (!inTeam[j]) return INF;
        if (!workers[j].can[op]) return INF;

        load[j] += tau[op];
        wsum[j] += tau[op] * w[op];
        usedSumW[j] += w[op];   // ops in request are unique
        cnt[j] += 1;
    }

    // every team member must have at least one task
    for (int j : team) {
        if (cnt[j] == 0) return INF;
    }

    // t_op
    double t_op = 0.0;
    for (int j : team) t_op = std::max(t_op, load[j]);

    // p2 and p4
    double p2_term = 0.0;
    double p4_term = 0.0;
    for (int j : team) {
        p2_term += wsum[j] / (double)cnt[j];
        p4_term += (start - avail[j]) * (double)cnt[j];
    }

    // alpha and p1
    double sumUsed = 0.0;
    for (int j : team) sumUsed += usedSumW[j];
    double sumAlpha = sumSkillsSumW - sumUsed;
    double p1_term = t_op * sumAlpha;

    double lc = P.p1 * p1_term
              + P.p2 * p2_term
              + travel_term
              + P.p3 * p3_term
              + P.p4 * p4_term;

    outTop = t_op;
    return lc;
}


// ============================================================
// Full genetic algorithm for one request (city + assignment)
// No team subset brute-force: chromosome chooses city and assignments.
// ============================================================

struct FullGAResult {
    bool feasible = false;
    Veci team;                   // global worker indices
    std::vector<int> bestAssign; // global worker index for each task position
    double bestLC = INF;
    double bestTop = INF;
    double bestDistKm = INF;
    double bestT2Hours = INF;
};

static FullGAResult solve_request_full_ga(
    Request const& r,
    std::vector<Worker> const& workers,
    Vecr const& tau,
    Vecr const& w,
    Vecr const& avail,
    Params const& P,
    DistCache& dist,
    int populationSize = 120,
    int generations = 300,
    double mutationProbAssign = 0.10,
    double mutationProbCity = 0.08
) {
    FullGAResult res;

    const int R = (int)r.tasks.size();
    const int K = (int)workers.size();
    if (R <= 0 || K <= 0) return res;

    // --- Group workers by city ---
    std::unordered_map<int, Veci> byCity;
    for (int j = 0; j < K; ++j) {
        byCity[workers[j].cityId].push_back(j);
    }

    // --- Build candidate cities and eligible lists ---
    // candidateCities[gidx] = real cityId
    std::vector<int> candidateCities;
    // cityWorkers[gidx] = global worker indices of that city
    std::vector<Veci> cityWorkers;
    // eligible[gidx][tpos] = local worker indices in cityWorkers[gidx]
    std::vector<std::vector<std::vector<int>>> eligibleAll;
    std::vector<double> distKmByG;
    std::vector<double> t2ByG;

    for (auto const& kv : byCity) {
        int cityId = kv.first;
        Veci const& gw = kv.second;

        if ((int)gw.size() > R) {
            // Необязательно отбрасывать: можно использовать подмножество.
            // Но в нашей кодировке бригада = только использованные работники,
            // так что это НЕ проблема. Город не отбрасываем.
        }

        double distKm = dist.get_km(cityId, r.cityId);
        if (!(distKm < INF)) continue;

        double t2h = distKm / P.speed_kmph;
        if (!(t2h < INF)) continue;

        std::vector<std::vector<int>> eligible(R);
        bool ok = true;
        for (int tpos = 0; tpos < R; ++tpos) {
            int op = r.tasks[tpos];
            for (int local = 0; local < (int)gw.size(); ++local) {
                int j = gw[local];
                if (workers[j].can[op]) eligible[tpos].push_back(local);
            }
            if (eligible[tpos].empty()) { ok = false; break; }
        }
        if (!ok) continue;

        candidateCities.push_back(cityId);
        cityWorkers.push_back(gw);
        eligibleAll.push_back(std::move(eligible));
        distKmByG.push_back(distKm);
        t2ByG.push_back(t2h);
    }

    if (candidateCities.empty()) return res;

    // --- RNG ---
    std::mt19937 rng((unsigned)std::random_device{}());
    std::uniform_real_distribution<double> U01(0.0, 1.0);

    // --- Chromosome ---
    struct Individual {
        int gidx = -1;              // index into candidateCities/cityWorkers/eligibleAll
        std::vector<int> chrom;     // chrom[tpos] = local worker index in cityWorkers[gidx]
        double lc = INF;
        double top = INF;
    };

    auto random_choice = [&](std::vector<int> const& v) -> int {
        std::uniform_int_distribution<int> d(0, (int)v.size() - 1);
        return v[d(rng)];
    };

    auto random_gidx = [&]() -> int {
        std::uniform_int_distribution<int> d(0, (int)candidateCities.size() - 1);
        return d(rng);
    };

    auto repair_for_city = [&](Individual& ind) {
        if (ind.gidx < 0 || ind.gidx >= (int)eligibleAll.size()) return;
        auto const& eligible = eligibleAll[ind.gidx];
        if ((int)ind.chrom.size() != R) ind.chrom.assign(R, 0);
        for (int tpos = 0; tpos < R; ++tpos) {
            bool valid = false;
            for (int loc : eligible[tpos]) {
                if (loc == ind.chrom[tpos]) { valid = true; break; }
            }
            if (!valid) {
                ind.chrom[tpos] = random_choice(eligible[tpos]);
            }
        }
    };

    auto eval = [&](Individual& ind) {
        ind.lc = INF;
        ind.top = INF;

        if (ind.gidx < 0 || ind.gidx >= (int)candidateCities.size()) return;
        if ((int)ind.chrom.size() != R) return;

        auto const& gw = cityWorkers[ind.gidx];

        // local -> global assign
        std::vector<int> assignGlobal(R, -1);
        for (int tpos = 0; tpos < R; ++tpos) {
            int loc = ind.chrom[tpos];
            if (loc < 0 || loc >= (int)gw.size()) return;
            assignGlobal[tpos] = gw[loc];
        }

        // team = unique workers from assignGlobal
        std::vector<char> used(K, 0);
        Veci team;
        team.reserve(std::min(R, (int)gw.size()));
        for (int j : assignGlobal) {
            if (!used[j]) {
                used[j] = 1;
                team.push_back(j);
            }
        }

        if (team.empty()) return;
        if ((int)team.size() > R) return;

        double top = INF;
        double lc = compute_LC_for_fixed_assignment(
            r, team, assignGlobal, workers, tau, w, avail, P, t2ByG[ind.gidx], top
        );
        if (!(lc < INF)) return;

        // tiny regularizer: prefer slightly smaller teams if LC equal-ish
        lc += 1e-6 * (double)team.size();

        ind.lc = lc;
        ind.top = top;
    };

    auto make_random_ind = [&]() {
        Individual ind;
        ind.gidx = random_gidx();
        ind.chrom.resize(R);
        auto const& eligible = eligibleAll[ind.gidx];
        for (int tpos = 0; tpos < R; ++tpos) {
            ind.chrom[tpos] = random_choice(eligible[tpos]);
        }
        eval(ind);
        return ind;
    };

    auto make_greedy_ind = [&]() {
        Individual ind;
        ind.gidx = random_gidx();

        // небольшая эвристика: иногда стартуем с города с минимальным t2
        if (U01(rng) < 0.5) {
            int best = 0;
            for (int g = 1; g < (int)t2ByG.size(); ++g) {
                if (t2ByG[g] < t2ByG[best]) best = g;
            }
            ind.gidx = best;
        }

        ind.chrom.assign(R, -1);
        auto const& gw = cityWorkers[ind.gidx];
        auto const& eligible = eligibleAll[ind.gidx];

        std::vector<double> localLoad(gw.size(), 0.0);

        for (int tpos = 0; tpos < R; ++tpos) {
            int op = r.tasks[tpos];
            int bestLoc = -1;
            double bestScore = INF;

            for (int loc : eligible[tpos]) {
                double score = localLoad[loc];
                if (score < bestScore) {
                    bestScore = score;
                    bestLoc = loc;
                }
            }

            if (bestLoc < 0) {
                ind.chrom[tpos] = random_choice(eligible[tpos]);
            } else {
                ind.chrom[tpos] = bestLoc;
                localLoad[bestLoc] += tau[op];
            }
        }

        eval(ind);
        return ind;
    };

    auto better = [](Individual const& a, Individual const& b) {
        return a.lc < b.lc;
    };

    auto select_tournament = [&](std::vector<Individual> const& pop, int k = 3) {
        std::uniform_int_distribution<int> d(0, (int)pop.size() - 1);
        int best = d(rng);
        for (int i = 1; i < k; ++i) {
            int c = d(rng);
            if (pop[c].lc < pop[best].lc) best = c;
        }
        return best;
    };

    auto crossover_mutate = [&](Individual const& a, Individual const& b) {
        Individual child;
        child.chrom.resize(R);

        // inherit city
        child.gidx = (U01(rng) < 0.5 ? a.gidx : b.gidx);

        // crossover assignments (before repair)
        if (R == 1) {
            child.chrom[0] = (U01(rng) < 0.5 ? a.chrom[0] : b.chrom[0]);
        } else {
            std::uniform_int_distribution<int> cutDist(1, R - 1);
            int cut = cutDist(rng);
            for (int t = 0; t < cut; ++t) child.chrom[t] = a.chrom[t];
            for (int t = cut; t < R; ++t) child.chrom[t] = b.chrom[t];
        }

        // mutate city
        if (U01(rng) < mutationProbCity) {
            child.gidx = random_gidx();
        }

        // repair because city may have changed
        repair_for_city(child);

        // mutate assignments within chosen city
        auto const& eligible = eligibleAll[child.gidx];
        for (int tpos = 0; tpos < R; ++tpos) {
            if (U01(rng) < mutationProbAssign) {
                child.chrom[tpos] = random_choice(eligible[tpos]);
            }
        }

        eval(child);
        return child;
    };

    // --- init population ---
    populationSize = std::max(8, populationSize);
    std::vector<Individual> pop;
    pop.reserve(populationSize);

    pop.push_back(make_greedy_ind());
    pop.push_back(make_greedy_ind());
    while ((int)pop.size() < populationSize) {
        pop.push_back(make_random_ind());
    }

    std::sort(pop.begin(), pop.end(), better);

    // --- evolution ---
    int stall = 0;
    double bestSeen = pop[0].lc;
    for (int gen = 0; gen < generations; ++gen) {
        std::vector<Individual> next;
        next.reserve(populationSize);

        // elitism
        next.push_back(pop[0]);
        next.push_back(pop[1]);

        while ((int)next.size() < populationSize) {
            int ia = select_tournament(pop);
            int ib = select_tournament(pop);
            next.push_back(crossover_mutate(pop[ia], pop[ib]));
        }

        std::sort(next.begin(), next.end(), better);
        pop.swap(next);
        
        if (pop[0].lc + 1e-12 < bestSeen) { bestSeen = pop[0].lc; stall = 0; }
        else stall++;

        if (stall >= 40) break; // например 40
    }

    // --- decode best ---
    if (pop.empty() || !(pop[0].lc < INF)) return res;

    Individual const& best = pop[0];
    auto const& gw = cityWorkers[best.gidx];

    std::vector<int> assignGlobal(R, -1);
    std::vector<char> used(K, 0);
    Veci team;

    for (int tpos = 0; tpos < R; ++tpos) {
        int loc = best.chrom[tpos];
        int j = gw[loc];
        assignGlobal[tpos] = j;
        if (!used[j]) {
            used[j] = 1;
            team.push_back(j);
        }
    }

    // recompute exact LC without tiny regularizer
    double top = INF;
    double exactLC = compute_LC_for_fixed_assignment(
        r, team, assignGlobal, workers, tau, w, avail, P, t2ByG[best.gidx], top
    );

    if (!(exactLC < INF)) return res;

    res.feasible = true;
    res.team = std::move(team);
    res.bestAssign = std::move(assignGlobal);
    res.bestLC = exactLC;
    res.bestTop = top;
    res.bestDistKm = distKmByG[best.gidx];
    res.bestT2Hours = t2ByG[best.gidx];
    return res;
}

// ============================================================
// Exact LC for a fixed team (full assignment enumeration)
// Also returns best assignment (worker per task position).
// ============================================================

// Model choices (as per your remarks):
// - r.tasks contains NO repeated operation types.
// - each worker in team must receive at least one task
// - all workers in team live in same city (enforced outside)
// - t2 is in hours: t2 = dist_km / speed_kmph
// - travel term: 2*t2 * sum(beta); with unique ops => sum(beta)=R
// - p3_term: sum_j avail[j] * |skills(j)|
// - p4_term: sum_j (start-avail[j]) * cnt[j]
static double compute_LC_team_min_over_assignments(
    Request const& r,
    Veci const& team,
    std::vector<Worker> const& workers,
    Vecr const& tau,
    Vecr const& w,
    Vecr const& avail,
    Params const& P,
    double t2_hours,
    double& outBestTop,
    std::vector<int>& outBestAssign // size R: outBestAssign[tpos] = workerIndex
) {
    if (team.empty()) return INF;

    const int R = (int)r.tasks.size();

    outBestTop = INF;
    outBestAssign.assign(R, -1);

    if ((int)team.size() > R) return INF; // cannot give everyone at least one task

    int K = (int)workers.size();

    // eligible workers for each task position
    std::vector<std::vector<int>> eligible(R);
    for (int t = 0; t < R; ++t) {
        int op = r.tasks[t];
        for (int j : team) if (workers[j].can[op]) eligible[t].push_back(j);
        if (eligible[t].empty()) return INF;
    }

    std::vector<int> cnt(K, 0);

    std::vector<int> curAssign(R, -1);
    std::vector<int> bestAssign(R, -1);

    double bestLC = INF;
    int assignedWorkers = 0;

    std::function<void(int)> dfs = [&](int tpos) {
        // pruning: must have enough remaining tasks to give at least one to each idle worker
        int remainTasks = R - tpos;
        int needWorkers = (int)team.size() - assignedWorkers;
        if (remainTasks < needWorkers) return;

        if (tpos == R) {
            if (assignedWorkers != (int)team.size()) return;

            double top = INF;
            double lc = compute_LC_for_fixed_assignment(
                r, team, curAssign, workers, tau, w, avail, P, t2_hours, top
            );

            if (lc < bestLC) {
                bestLC = lc;
                outBestTop = top;
                bestAssign = curAssign;
            }
            return;
        }

        for (int j : eligible[tpos]) {
            bool first = (cnt[j] == 0);
            if (first) assignedWorkers++;

            curAssign[tpos] = j;

            cnt[j]  += 1;

            dfs(tpos + 1);

            cnt[j]  -= 1;

            curAssign[tpos] = -1;

            if (first) assignedWorkers--;
        }
    };

    dfs(0);

    if (bestLC < INF) outBestAssign = bestAssign;
    return bestLC;
}

// ============================================================
// Team selection: brute force over all subsets satisfying constraints
// Returns best team + best assignment + bestTop + bestLC
// ============================================================

struct BestTeamResult {
    Veci team;
    std::vector<int> bestAssign; // size R, maps each task position to worker index
    double bestLC = INF;
    double bestTop = INF;
    double bestDistKm = INF;
    double bestT2Hours = INF;
};

static BestTeamResult build_team_bruteforce(
    Request const& r,
    std::vector<Worker> const& workers,
    Vecr const& tau,
    Vecr const& w,
    Vecr const& avail,
    Params const& P,
    DistCache& dist
) {
    BestTeamResult out;
    int K = (int)workers.size();

    Veci curTeam;
    curTeam.reserve(K);

    std::function<void(int,int)> dfs = [&](int idx, int fixedCityId) {
        if (idx == K) {
            if (curTeam.empty()) return;
            if ((int)curTeam.size() > (int)r.tasks.size()) return;

            // must cover
            if (!covers_all(r, curTeam, workers)) return;

            // compute routing time from team city to request city
            int teamCityId = fixedCityId;
            double distKm = dist.get_km(teamCityId, r.cityId);
            if (!(distKm < INF)) return;
            double t2_hours = distKm / P.speed_kmph;

            double top = INF;
            std::vector<int> bestAssign;
            double lc = compute_LC_team_min_over_assignments(
                r, curTeam, workers, tau, w, avail, P, t2_hours, top, bestAssign
            );

            if (lc < out.bestLC) {
                out.bestLC = lc;
                out.bestTop = top;
                out.team = curTeam;
                out.bestAssign = bestAssign;
                out.bestDistKm = distKm;
                out.bestT2Hours = t2_hours;
            }
            return;
        }

        // option 1: skip idx
        dfs(idx + 1, fixedCityId);

        // option 2: take idx (must satisfy same-city constraint)
        int cid = workers[idx].cityId;
        if (curTeam.empty()) {
            curTeam.push_back(idx);
            dfs(idx + 1, cid);
            curTeam.pop_back();
        } else {
            if (cid == fixedCityId) {
                curTeam.push_back(idx);
                dfs(idx + 1, fixedCityId);
                curTeam.pop_back();
            }
        }
    };

    dfs(0, -1);
    return out;
}

// ============================================================
// Public solver
// ============================================================

Solution solve_with_roads(
    std::vector<RequestInput> const& requestsIn,
    std::vector<WorkerInput>  const& workersIn,
    Vecr const& tau,
    std::vector<RoadEdge> const& roads,
    Params const& P,
    Veci order,
    bool dynamicW
) {
    if (P.speed_kmph <= 0.0) throw std::runtime_error("speed_kmph must be > 0");
    int N = (int)tau.size();
    if (N == 0) throw std::runtime_error("tau is empty");

    // --- build city ids ---
    std::unordered_map<std::string,int> cityToId;
    std::vector<std::string> idToCity;

    auto getCityId = [&](std::string const& name) -> int {
        auto it = cityToId.find(name);
        if (it != cityToId.end()) return it->second;
        int id = (int)idToCity.size();
        cityToId[name] = id;
        idToCity.push_back(name);
        return id;
    };

    for (auto const& e : roads) {
        getCityId(e.from);
        getCityId(e.to);
    }
    for (auto const& w0 : workersIn) getCityId(w0.city);
    for (auto const& r0 : requestsIn) getCityId(r0.city);

    int C = (int)idToCity.size();
    CityGraph g;
    g.adj.assign(C, {});
    for (auto const& e : roads) {
        int u = getCityId(e.from);
        int v = getCityId(e.to);
        if (e.km < 0) throw std::runtime_error("Negative road distance is not allowed");
        add_undirected_edge(g, u, v, e.km);
    }

    DistCache dist(std::move(g));

    // --- convert workers ---
    std::vector<Worker> workers;
    workers.reserve(workersIn.size());
    for (auto const& wi : workersIn) {
        Worker wkr;
        wkr.name = wi.name;
        wkr.cityId = getCityId(wi.city);
        wkr.skills = wi.skills;
        wkr.can.assign(N, 0);
        for (int op : wi.skills) {
            if (op < 0 || op >= N) throw std::runtime_error("Worker skill op out of range");
            wkr.can[op] = 1;
        }
        workers.push_back(std::move(wkr));
    }

    // --- convert requests ---
    std::vector<Request> reqs;
    reqs.reserve(requestsIn.size());
    for (auto const& ri : requestsIn) {
        Request rq;
        rq.id = ri.id;
        rq.cityId = getCityId(ri.city);
        rq.tasks = ri.tasks;
        for (int op : rq.tasks) {
            if (op < 0 || op >= N) throw std::runtime_error("Request task op out of range");
        }
        reqs.push_back(std::move(rq));
    }

    int M = (int)reqs.size();
    int K = (int)workers.size();

    // --- default order ---
    if (order.empty()) {
        order.resize(M);
        for (int i = 0; i < M; ++i) order[i] = i;
    }

    // --- compute w if not provided ---
    Vecr w = compute_w(reqs, workers, N);

    // --- availability times ---
    Vecr avail(K, 0.0);

    Solution sol;
    sol.perRequest.resize(M);

    for (int pos = 0; pos < (int)order.size(); ++pos) {
        int idx = order[pos];
        if (idx < 0 || idx >= M) throw std::runtime_error("order contains invalid request index");

        if (dynamicW) {
            // recompute w using remaining requests (simple dynamic variant)
            std::vector<Request> remaining;
            remaining.reserve(order.size() - pos);
            for (int t = pos; t < (int)order.size(); ++t) remaining.push_back(reqs[order[t]]);
            w = compute_w(remaining.empty() ? reqs : remaining, workers, N);
        }

        Request const& r = reqs[idx];

        RequestSolution rs;
        rs.requestId = r.id;
        rs.assignment.assign(K, {});

        BestTeamResult best = build_team_bruteforce(r, workers, tau, w, avail, P, dist);

        if (!(best.bestLC < INF) || best.team.empty()) {
            rs.feasible = false;
            rs.lc = INF;
            rs.startTime = 0.0;
            rs.finishTime = INF;
            sol.perRequest[idx] = std::move(rs);
            continue;
        }

        // build assignment vectors from best.bestAssign
        for (int tpos = 0; tpos < (int)r.tasks.size(); ++tpos) {
            int op = r.tasks[tpos];
            int wj = best.bestAssign[tpos];
            if (wj < 0) continue;
            rs.assignment[wj].push_back(op);
        }

        // compute start & finish
        double start = 0.0;
        for (int j : best.team) start = std::max(start, avail[j]);
        double finish = start + 2.0 * best.bestT2Hours + best.bestTop;

        for (int j : best.team) avail[j] = finish;

        rs.team = best.team;
        rs.lc = best.bestLC;
        rs.startTime = start;
        rs.finishTime = finish;
        rs.distKm = best.bestDistKm;
        rs.t2Hours = best.bestT2Hours;
        rs.feasible = true;

        sol.perRequest[idx] = std::move(rs);
    }

    return sol;
}


Solution solve_with_roads_full_ga(
    std::vector<RequestInput> const& requestsIn,
    std::vector<WorkerInput>  const& workersIn,
    Vecr const& tau,
    std::vector<RoadEdge> const& roads,
    Params const& P,
    Veci order,
    bool dynamicW
) {
    if (P.speed_kmph <= 0.0) throw std::runtime_error("speed_kmph must be > 0");
    int N = (int)tau.size();
    if (N == 0) throw std::runtime_error("tau is empty");

    // --- build city ids ---
    std::unordered_map<std::string,int> cityToId;
    std::vector<std::string> idToCity;

    auto getCityId = [&](std::string const& name) -> int {
        auto it = cityToId.find(name);
        if (it != cityToId.end()) return it->second;
        int id = (int)idToCity.size();
        cityToId[name] = id;
        idToCity.push_back(name);
        return id;
    };

    for (auto const& e : roads) {
        getCityId(e.from);
        getCityId(e.to);
    }
    for (auto const& w0 : workersIn) getCityId(w0.city);
    for (auto const& r0 : requestsIn) getCityId(r0.city);

    int C = (int)idToCity.size();
    CityGraph g;
    g.adj.assign(C, {});
    for (auto const& e : roads) {
        int u = getCityId(e.from);
        int v = getCityId(e.to);
        if (e.km < 0) throw std::runtime_error("Negative road distance is not allowed");
        add_undirected_edge(g, u, v, e.km);
    }

    DistCache dist(std::move(g));

    // --- convert workers ---
    std::vector<Worker> workers;
    workers.reserve(workersIn.size());
    for (auto const& wi : workersIn) {
        Worker wkr;
        wkr.name = wi.name;
        wkr.cityId = getCityId(wi.city);
        wkr.skills = wi.skills;
        wkr.can.assign(N, 0);
        for (int op : wi.skills) {
            if (op < 0 || op >= N) throw std::runtime_error("Worker skill op out of range");
            wkr.can[op] = 1;
        }
        workers.push_back(std::move(wkr));
    }

    // --- convert requests ---
    std::vector<Request> reqs;
    reqs.reserve(requestsIn.size());
    for (auto const& ri : requestsIn) {
        Request rq;
        rq.id = ri.id;
        rq.cityId = getCityId(ri.city);
        rq.tasks = ri.tasks;
        for (int op : rq.tasks) {
            if (op < 0 || op >= N) throw std::runtime_error("Request task op out of range");
        }
        reqs.push_back(std::move(rq));
    }

    int M = (int)reqs.size();
    int K = (int)workers.size();

    if (order.empty()) {
        order.resize(M);
        for (int i = 0; i < M; ++i) order[i] = i;
    }

    Vecr w = compute_w(reqs, workers, N);
    Vecr avail(K, 0.0);

    Solution sol;
    sol.perRequest.resize(M);

    for (int pos = 0; pos < (int)order.size(); ++pos) {
        int idx = order[pos];
        if (idx < 0 || idx >= M) throw std::runtime_error("order contains invalid request index");

        if (dynamicW) {
            std::vector<Request> remaining;
            remaining.reserve(order.size() - pos);
            for (int t = pos; t < (int)order.size(); ++t) remaining.push_back(reqs[order[t]]);
            w = compute_w(remaining.empty() ? reqs : remaining, workers, N);
        }

        Request const& r = reqs[idx];

        RequestSolution rs;
        rs.requestId = r.id;
        rs.assignment.assign(K, {});

        int R = (int)r.tasks.size();
        int pop = (R <= 3 ? 20 : R <= 6 ? 40 : R <= 10 ? 60 : 80);
        int gen = (R <= 3 ? 40 : R <= 6 ? 80 : R <= 10 ? 120 : 180);

        FullGAResult best = solve_request_full_ga(
            r, workers, tau, w, avail, P, dist,
            pop, gen, 0.10, 0.08
        );

        if (!best.feasible || !(best.bestLC < INF) || best.team.empty()) {
            rs.feasible = false;
            rs.lc = INF;
            rs.startTime = 0.0;
            rs.finishTime = INF;
            sol.perRequest[idx] = std::move(rs);
            continue;
        }

        // assignment[workerIndex] = list of operation types
        for (int tpos = 0; tpos < (int)r.tasks.size(); ++tpos) {
            int op = r.tasks[tpos];
            int wj = best.bestAssign[tpos];
            if (wj >= 0) rs.assignment[wj].push_back(op);
        }

        double start = 0.0;
        for (int j : best.team) start = std::max(start, avail[j]);

        double finish = start + 2.0 * best.bestT2Hours + best.bestTop;
        for (int j : best.team) avail[j] = finish;

        rs.team = best.team;
        rs.lc = best.bestLC;
        rs.startTime = start;
        rs.finishTime = finish;
        rs.distKm = best.bestDistKm;
        rs.t2Hours = best.bestT2Hours;
        rs.feasible = true;

        sol.perRequest[idx] = std::move(rs);
    }

    return sol;
}
