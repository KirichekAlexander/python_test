#include "brigade_routing.h"

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

    const int N = (int)w.size();
    (void)N;
    const int R = (int)r.tasks.size();

    outBestTop = INF;
    outBestAssign.assign(R, -1);

    if ((int)team.size() > R) return INF; // cannot give everyone at least one task

    // start time for this team
    double start = 0.0;
    for (int j : team) start = std::max(start, avail[j]);

    // p3_term = sum(avail[j] * |skills(j)|)
    double p3_term = 0.0;
    for (int j : team) p3_term += avail[j] * (double)workers[j].skills.size();

    // travel term per PDF: 2*t2 * sum(beta) ; for unique ops sum(beta)=R
    if (!(t2_hours < INF)) return INF; // unreachable
    double travel_term = 2.0 * t2_hours * (double)R;

    // skillsSumW[j] and sum over team
    int K = (int)workers.size();
    std::vector<double> skillsSumW(K, 0.0);
    double sumSkillsSumW = 0.0;
    for (int j : team) {
        double s = 0.0;
        for (int skill : workers[j].skills) s += w[skill];
        skillsSumW[j] = s;
        sumSkillsSumW += s;
    }

    // eligible workers for each task position
    std::vector<std::vector<int>> eligible(R);
    for (int t = 0; t < R; ++t) {
        int op = r.tasks[t];
        for (int j : team) if (workers[j].can[op]) eligible[t].push_back(j);
        if (eligible[t].empty()) return INF;
    }

    // state for DFS
    std::vector<double> load(K, 0.0);      // sum tau
    std::vector<double> wsum(K, 0.0);      // sum tau*w
    std::vector<double> usedSumW(K, 0.0);  // since ops are unique: usedSumW[j] = sum w[op assigned to j]
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

            double t_op = 0.0;
            for (int j : team) t_op = std::max(t_op, load[j]);

            double p2_term = 0.0;
            double p4_term = 0.0;
            for (int j : team) {
                // cnt[j] >= 1
                p2_term += wsum[j] / (double)cnt[j];
                p4_term += (start - avail[j]) * (double)cnt[j];
            }

            double sumUsed = 0.0;
            for (int j : team) sumUsed += usedSumW[j];
            double sumAlpha = sumSkillsSumW - sumUsed;
            double p1_term = t_op * sumAlpha;

            double lc = P.p1 * p1_term
                      + P.p2 * p2_term
                      + travel_term
                      + P.p3 * p3_term
                      + P.p4 * p4_term;

            if (lc < bestLC) {
                bestLC = lc;
                outBestTop = t_op;
                bestAssign = curAssign;
            }
            return;
        }

        int op = r.tasks[tpos];
        for (int j : eligible[tpos]) {
            bool first = (cnt[j] == 0);
            if (first) assignedWorkers++;

            curAssign[tpos] = j;

            load[j] += tau[op];
            wsum[j] += tau[op] * w[op];
            cnt[j]  += 1;
            usedSumW[j] += w[op];

            dfs(tpos + 1);

            usedSumW[j] -= w[op];
            cnt[j]  -= 1;
            wsum[j] -= tau[op] * w[op];
            load[j] -= tau[op];

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

