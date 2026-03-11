import os
import time
import csv
import random
import statistics as stats

import calc_module as cm
from rus_roads_data import get_rus_roads

# -----------------------------
# metrics
# -----------------------------

def compute_lc_stats(sol):
    vals = [rs.lc for rs in sol.perRequest if rs.feasible]
    if not vals:
        return {"sum_lc": 0.0, "mean_lc": 0.0, "median_lc": 0.0}
    return {"sum_lc": sum(vals), "mean_lc": sum(vals)/len(vals), "median_lc": stats.median(vals)}

def compute_busy_breakdown_both(sol, num_workers: int):
    total = [0.0] * num_workers
    travel = [0.0] * num_workers
    work = [0.0] * num_workers

    feasible_cnt = 0
    makespan = 0.0

    for rs in sol.perRequest:
        if not rs.feasible:
            continue
        feasible_cnt += 1
        makespan = max(makespan, rs.finishTime)

        dur_total = rs.finishTime - rs.startTime
        dur_travel = 2.0 * rs.t2Hours
        dur_work = dur_total - dur_travel
        if dur_work < 0 and dur_work > -1e-9:
            dur_work = 0.0

        for j in rs.team:
            total[j] += dur_total
            travel[j] += dur_travel
            work[j] += dur_work

    all_idx = list(range(num_workers))
    active_idx = [j for j in all_idx if total[j] > 0.0]

    def mean_median(arr, idxs):
        if not idxs:
            return 0.0, 0.0
        vals = [arr[j] for j in idxs]
        return (sum(vals)/len(vals), stats.median(vals))

    def mean_median_share(idxs, include_zeros):
        if not idxs:
            return 0.0, 0.0
        shares = []
        for j in idxs:
            if total[j] > 0:
                shares.append(travel[j]/total[j])
            else:
                if include_zeros:
                    shares.append(0.0)
        return (sum(shares)/len(shares), stats.median(shares))

    # active
    a_mean_total, a_med_total = mean_median(total, active_idx)
    a_mean_tr, a_med_tr = mean_median(travel, active_idx)
    a_mean_w, a_med_w = mean_median(work, active_idx)
    a_mean_sh, a_med_sh = mean_median_share(active_idx, include_zeros=False)

    # all
    all_mean_total, all_med_total = mean_median(total, all_idx)
    all_mean_tr, all_med_tr = mean_median(travel, all_idx)
    all_mean_w, all_med_w = mean_median(work, all_idx)
    all_mean_sh, all_med_sh = mean_median_share(all_idx, include_zeros=True)

    return {
        "active_count": len(active_idx),
        "feasible": feasible_cnt,
        "total_requests": len(sol.perRequest),
        "makespan": makespan,

        "active_mean_total": a_mean_total,
        "active_median_total": a_med_total,
        "active_mean_travel": a_mean_tr,
        "active_median_travel": a_med_tr,
        "active_mean_work": a_mean_w,
        "active_median_work": a_med_w,
        "active_mean_travel_share": a_mean_sh,
        "active_median_travel_share": a_med_sh,

        "all_mean_total": all_mean_total,
        "all_median_total": all_med_total,
        "all_mean_travel": all_mean_tr,
        "all_median_travel": all_med_tr,
        "all_mean_work": all_mean_w,
        "all_median_work": all_med_w,
        "all_mean_travel_share": all_mean_sh,
        "all_median_travel_share": all_med_sh,
    }

# -----------------------------
# benchmarking
# -----------------------------

def bench(fn, args, repeats=1, warmup=2):
    for _ in range(warmup):
        fn(*args)
    ts = []
    for _ in range(repeats):
        t0 = time.perf_counter()
        sol = fn(*args)
        t1 = time.perf_counter()
        ts.append(t1 - t0)
    return ts

def time_stats(times):
    return {
        "time_mean_s": stats.mean(times),
        "time_median_s": stats.median(times),
        "time_min_s": min(times),
        "time_max_s": max(times),
    }

# -----------------------------
# dataset generation
# -----------------------------

def mk_worker(name, city, skills):
    return cm.WorkerInput(name, city, list(skills))

def mk_request(rid, city, tasks):
    return cm.RequestInput(int(rid), city, list(tasks))

def pick_requests(rng, cities, num_requests, R_min, R_max, num_ops):
    reqs = []
    for rid in range(num_requests):
        city = rng.choice(cities)
        R = rng.randint(R_min, R_max)
        tasks = rng.sample(range(num_ops), k=R)
        reqs.append(mk_request(rid, city, tasks))
    return reqs

def make_workers_for_city(rng, city, count, num_ops, skill_min, skill_max, prefix):
    workers = []
    for i in range(count):
        s = rng.randint(skill_min, skill_max)
        skills = rng.sample(range(num_ops), k=s)
        workers.append(mk_worker(f"{prefix}{i}", city, skills))
    return workers

def build_dataset(rng_seed, dataset_name, num_ops, tau, base_cities, worker_counts, request_cities, num_requests, R_min, R_max):
    rng = random.Random(rng_seed)
    roads = get_rus_roads(cm)

    # workers
    workers = []
    for city, cnt in zip(base_cities, worker_counts):
        workers += make_workers_for_city(
            rng, city, cnt, num_ops=num_ops,
            skill_min=max(1, num_ops//2),
            skill_max=num_ops,
            prefix=city[:2].upper() + "_"
        )

    # requests
    requests = pick_requests(rng, request_cities, num_requests, R_min, R_max, num_ops=num_ops)

    params = cm.CrewParams()
    params.speed_kmph = 60.0
    params.p1 = params.p2 = params.p3 = params.p4 = 1.0

    order = list(range(len(requests))) 

    return {
        "name": dataset_name,
        "tau": tau,
        "roads": roads,
        "workers": workers,
        "requests": requests,
        "params": params,
        "order": order,
    }

def make_5_datasets():
    # базовые города
    bases = ["Москва", "Санкт-Петербург", "Севастополь", "Хабаровск"]
    # города заявок
    req_cities = ["Екатеринбург", "Челябинск", "Уфа", "Пермь", "Казань", "Самара", "Саратов", "Волгоград", "Краснодар", "Ростов", "Иркутск"]

    datasets = []

    # D1: маленький
    datasets.append(build_dataset(
        rng_seed=1,
        dataset_name="D1_small",
        num_ops=3,
        tau=[2.0, 3.0, 5.0],
        base_cities=bases,
        worker_counts=[3, 3, 3, 3],
        request_cities=req_cities,
        num_requests=12,
        R_min=1, R_max=3
    ))

    # D2: чуть сложнее
    datasets.append(build_dataset(
        rng_seed=2,
        dataset_name="D2_medium",
        num_ops=5,
        tau=[2.0, 3.0, 5.0, 4.0, 6.0],
        base_cities=bases,
        worker_counts=[8, 8, 6, 6],
        request_cities=req_cities,
        num_requests=25,
        R_min=2, R_max=4
    ))

    # D3: много заявок + больше операций
    datasets.append(build_dataset(
        rng_seed=3,
        dataset_name="D3_medplus",
        num_ops=7,
        tau=[2.0, 3.0, 5.0, 4.0, 6.0, 3.5, 7.0],
        base_cities=bases,
        worker_counts=[15, 12, 10, 10],
        request_cities=req_cities,
        num_requests=40,
        R_min=3, R_max=5
    ))

    # D4: сложный
    datasets.append(build_dataset(
        rng_seed=4,
        dataset_name="D4_large",
        num_ops=9,
        tau=[2.0, 3.0, 5.0, 4.0, 6.0, 3.5, 7.0, 5.5, 8.0],
        base_cities=bases,
        worker_counts=[30, 25, 20, 20],
        request_cities=req_cities,
        num_requests=60,
        R_min=4, R_max=7
    ))

    # D5: стресс
    datasets.append(build_dataset(
        rng_seed=5,
        dataset_name="D5_stress",
        num_ops=12,
        tau=[2.0, 3.0, 5.0, 4.0, 6.0, 3.5, 7.0, 5.5, 8.0, 4.5, 6.5, 7.5],
        base_cities=bases,
        worker_counts=[45, 35, 30, 30],
        request_cities=req_cities,
        num_requests=80,
        R_min=6, R_max=10
    ))

    return datasets

# -----------------------------
# runner
# -----------------------------

def run():
    datasets = make_5_datasets()
    datasets = datasets [: 3 ]

    out_path = os.path.join(os.path.dirname(__file__), "compare_summary.csv")

    fieldnames = [
        "dataset","method",
        "time_mean_s","time_median_s","time_min_s","time_max_s",
        "active_count","feasible","total","makespan",
        "sum_lc","mean_lc","median_lc",
        "active_mean_total","active_median_total","active_mean_travel","active_median_travel",
        "active_mean_work","active_median_work","active_mean_travel_share","active_median_travel_share",
        "all_mean_total","all_median_total","all_mean_travel","all_median_travel",
        "all_mean_work","all_median_work","all_mean_travel_share","all_median_travel_share",
    ]

    methods = [
        ("EXACT", cm.solve_crew_routing),

        # GA: 3 режима целевой функции
        ("GA_LC",       lambda *a: cm.solve_crew_routing_ga(*a, objective=cm.GAObjective.LC)),
        ("GA_FINISH",   lambda *a: cm.solve_crew_routing_ga(*a, objective=cm.GAObjective.FINISH)),
        ("GA_DURATION", lambda *a: cm.solve_crew_routing_ga(*a, objective=cm.GAObjective.DURATION)),

        ("FREE", cm.solve_crew_routing_free),
    ]

    with open(out_path, "w", newline="", encoding="utf-8") as f:
        wcsv = csv.DictWriter(f, fieldnames=fieldnames, extrasaction="ignore")
        wcsv.writeheader()

        solutions_dir = os.path.join(os.path.dirname(__file__), "solutions")
        os.makedirs(solutions_dir, exist_ok=True)
        for ds in datasets:
            args = (ds["requests"], ds["workers"], ds["tau"], ds["roads"], ds["params"], ds["order"], False)

            for mname, mfn in methods:
                # замер времени
                times = bench(mfn, args, repeats=1, warmup=2)
                tstat = time_stats(times)

                sol = mfn(*args)
                # сохранить подробное решение в CSV через C++ функцию
                sol_file = os.path.join(solutions_dir, f"{ds['name']}_{mname}.csv")
                cm.save_crew_routing_solution_csv(sol_file, sol)

                busy = compute_busy_breakdown_both(sol, num_workers=len(ds["workers"]))
                lc = compute_lc_stats(sol)

                row = {
                    "dataset": ds["name"],
                    "method": mname,
                    **tstat,
                    "active_count": busy["active_count"],
                    "feasible": busy["feasible"],
                    "total": busy["total_requests"],
                    "makespan": busy["makespan"],
                    **lc,
                    **busy,
                }
                
                wcsv.writerow(row)

                print(ds["name"], mname, "time_median_s=", f"{tstat['time_median_s']:.6f}")

    print("Saved:", out_path)

if __name__ == "__main__":
    run()