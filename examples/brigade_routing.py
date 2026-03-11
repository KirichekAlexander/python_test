import calc_module as cm
from rus_roads_data import get_rus_roads

import csv
import statistics as stats

def compute_lc_stats(sol):
    lc_vals = [rs.lc for rs in sol.perRequest if rs.feasible]
    if not lc_vals:
        return {"sum_lc": 0.0, "mean_lc": 0.0, "median_lc": 0.0}
    return {
        "sum_lc": sum(lc_vals),
        "mean_lc": sum(lc_vals) / len(lc_vals),
        "median_lc": stats.median(lc_vals),
    }

def compute_busy_breakdown_both(sol, num_workers: int):
    total = [0.0] * num_workers
    travel = [0.0] * num_workers
    work = [0.0] * num_workers

    feasible_cnt = 0
    max_finish = 0.0

    for rs in sol.perRequest:
        if not rs.feasible:
            continue
        feasible_cnt += 1
        max_finish = max(max_finish, rs.finishTime)

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
        return (sum(vals) / len(vals), stats.median(vals))

    def mean_median_share(idxs, include_zeros: bool):
        if not idxs:
            return 0.0, 0.0
        shares = []
        for j in idxs:
            if total[j] > 0:
                shares.append(travel[j] / total[j])
            else:
                if include_zeros:
                    shares.append(0.0)   # для all: неактивные -> 0
                # для active сюда не попадём
        return (sum(shares) / len(shares), stats.median(shares))

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
        "makespan": max_finish,

        # active (8)
        "active_mean_total": a_mean_total,
        "active_median_total": a_med_total,
        "active_mean_travel": a_mean_tr,
        "active_median_travel": a_med_tr,
        "active_mean_work": a_mean_w,
        "active_median_work": a_med_w,
        "active_mean_travel_share": a_mean_sh,
        "active_median_travel_share": a_med_sh,

        # all (8)
        "all_mean_total": all_mean_total,
        "all_median_total": all_med_total,
        "all_mean_travel": all_mean_tr,
        "all_median_travel": all_med_tr,
        "all_mean_work": all_mean_w,
        "all_median_work": all_med_w,
        "all_mean_travel_share": all_mean_sh,
        "all_median_travel_share": all_med_sh,
    }

def save_compare_two_both_csv(filename: str, method_a: str, sol_a, method_b: str, sol_b, num_workers: int):
    a = compute_busy_breakdown_both(sol_a, num_workers)
    b = compute_busy_breakdown_both(sol_b, num_workers)

    a_lc = compute_lc_stats(sol_a)
    b_lc = compute_lc_stats(sol_b)

    fieldnames = [
        "method","active_count","feasible","total","makespan",
        # LC summary
        "sum_lc","mean_lc","median_lc",
        # active (8)
        "active_mean_total","active_median_total","active_mean_travel","active_median_travel",
        "active_mean_work","active_median_work","active_mean_travel_share","active_median_travel_share",
        # all (8)
        "all_mean_total","all_median_total","all_mean_travel","all_median_travel",
        "all_mean_work","all_median_work","all_mean_travel_share","all_median_travel_share",
    ]

    def row(method, s, lc):
        return {
            "method": method,
            "active_count": s["active_count"],
            "feasible": s["feasible"],
            "total": s["total_requests"],
            "makespan": s["makespan"],

            "sum_lc": lc["sum_lc"],
            "mean_lc": lc["mean_lc"],
            "median_lc": lc["median_lc"],

            "active_mean_total": s["active_mean_total"],
            "active_median_total": s["active_median_total"],
            "active_mean_travel": s["active_mean_travel"],
            "active_median_travel": s["active_median_travel"],
            "active_mean_work": s["active_mean_work"],
            "active_median_work": s["active_median_work"],
            "active_mean_travel_share": s["active_mean_travel_share"],
            "active_median_travel_share": s["active_median_travel_share"],

            "all_mean_total": s["all_mean_total"],
            "all_median_total": s["all_median_total"],
            "all_mean_travel": s["all_mean_travel"],
            "all_median_travel": s["all_median_travel"],
            "all_mean_work": s["all_mean_work"],
            "all_median_work": s["all_median_work"],
            "all_mean_travel_share": s["all_mean_travel_share"],
            "all_median_travel_share": s["all_median_travel_share"],
        }

    with open(filename, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        w.writerow(row(method_a, a, a_lc))
        w.writerow(row(method_b, b, b_lc))

    print("Saved:", filename)


def mk_worker(name, city, skills):
    return cm.WorkerInput(name, city, list(skills))

def mk_request(rid, city, tasks):
    return cm.RequestInput(int(rid), city, list(tasks))

def vec(v):
    return "[" + ", ".join(str(x) for x in v) + "]"

def main():
    # время на выполнение каждой задачи
    tau = [2.0, 3.0, 5.0]

    # дороги из отдельного python файла
    roads = get_rus_roads(cm)

    # 4 базы: Москва, Санкт-Петербург, Севастополь, Хабаровск
    workers = [
        # Москва
        mk_worker("M1", "Москва", [0, 1]),
        mk_worker("M2", "Москва", [1, 2]),
        mk_worker("M3", "Москва", [0, 2]),

        # Санкт-Петербург
        mk_worker("SP1", "Санкт-Петербург", [0, 1, 2]),
        mk_worker("SP2", "Санкт-Петербург", [0]),
        mk_worker("SP3", "Санкт-Петербург", [2]),

        # Севастополь
        mk_worker("SV1", "Севастополь", [0, 1]),
        mk_worker("SV2", "Севастополь", [1]),
        mk_worker("SV3", "Севастополь", [2]),

        # Хабаровск
        mk_worker("KH1", "Хабаровск", [0, 2]),
        mk_worker("KH2", "Хабаровск", [1, 2]),
        mk_worker("KH3", "Хабаровск", [0, 1]),
    ]

    # 11 объектов (Урал + Юг + Сибирь)
    requests = [
        mk_request(0, "Екатеринбург", [0, 1]),
        mk_request(1, "Челябинск",    [1, 2]),
        mk_request(2, "Уфа",          [0, 2]),
        mk_request(3, "Пермь",        [0]),
        mk_request(4, "Казань",       [2]),
        mk_request(5, "Самара",       [0, 1, 2]),
        mk_request(6, "Саратов",      [1]),
        mk_request(7, "Волгоград",    [0, 2]),
        mk_request(8, "Краснодар",    [1, 2]),
        mk_request(9, "Ростов",       [0, 1]),
        mk_request(10, "Иркутск",      [0, 1]),
    ]

    params = cm.CrewParams()
    params.speed_kmph = 60.0
    params.p1 = params.p2 = params.p3 = params.p4 = 1.0

    order = list(range(len(requests)))


    sol_exact = cm.solve_crew_routing(requests, workers, tau, roads, params, order, False)
    sol_ga    = cm.solve_crew_routing_ga(requests, workers, tau, roads, params, order, False)

    save_compare_two_both_csv(
        "compare_summary.csv",
        "EXACT", sol_exact,
        "GA", sol_ga,
        num_workers=len(workers)
    )

    # for rs in sol.perRequest:
    #     print(f"Заявка id={rs.requestId} | выполнима={'да' if rs.feasible else 'нет'}")
    #     if not rs.feasible:
    #         print("  (нет решения)\n")
    #         continue

    #     print(f"  Бригада (индексы): {vec(list(rs.team))}")
    #     print(f"  distKm={rs.distKm:.3f} | t2Hours={rs.t2Hours:.3f} | start={rs.startTime:.3f} | finish={rs.finishTime:.3f} | LC={rs.lc:.3f}")
    #     print("  Назначение:")
    #     for j in rs.team:
    #         ops = rs.assignment[j]
    #         print(f"    worker #{j} ({workers[j].name}, {workers[j].city}): {vec(list(ops))}")
    #     print()

if __name__ == "__main__":
    main()



# import time
# import statistics as stats
# import calc_module as cm
# from rus_roads_data import get_rus_roads

# def bench(fn, args, repeats=10, warmup=2):
#     # прогрев
#     for _ in range(warmup):
#         fn(*args)

#     times = []
#     for _ in range(repeats):
#         t0 = time.perf_counter()
#         fn(*args)
#         t1 = time.perf_counter()
#         times.append(t1 - t0)
#     return times

# def main():
#     # --- твои данные (примерно как у тебя) ---
#     tau = [2.0, 3.0, 5.0]
#     roads = get_rus_roads(cm)

#     workers = [
#         cm.WorkerInput("M1", "Москва", [0,1]),
#         cm.WorkerInput("M2", "Москва", [1,2]),
#         cm.WorkerInput("M3", "Москва", [0,2]),
#         cm.WorkerInput("SP1", "Санкт-Петербург", [0,1,2]),
#         cm.WorkerInput("SP2", "Санкт-Петербург", [0]),
#         cm.WorkerInput("SP3", "Санкт-Петербург", [2]),
#         cm.WorkerInput("SV1", "Севастополь", [0,1]),
#         cm.WorkerInput("SV2", "Севастополь", [1]),
#         cm.WorkerInput("SV3", "Севастополь", [2]),
#         cm.WorkerInput("KH1", "Хабаровск", [0,2]),
#         cm.WorkerInput("KH2", "Хабаровск", [1,2]),
#         cm.WorkerInput("KH3", "Хабаровск", [0,1]),
#     ]

#     requests = [
#         cm.RequestInput(0, "Екатеринбург", [0,1]),
#         cm.RequestInput(1, "Челябинск", [1,2]),
#         cm.RequestInput(2, "Уфа", [0,2]),
#         cm.RequestInput(3, "Пермь", [0]),
#         cm.RequestInput(4, "Казань", [2]),
#         cm.RequestInput(5, "Самара", [0,1,2]),
#         cm.RequestInput(6, "Саратов", [1]),
#         cm.RequestInput(7, "Волгоград", [0,2]),
#         cm.RequestInput(8, "Краснодар", [1,2]),
#         cm.RequestInput(9, "Ростов", [0,1]),
#         cm.RequestInput(10, "Иркутск", [0,1]),
#     ]

#     params = cm.CrewParams()
#     params.speed_kmph = 60.0
#     params.p1 = params.p2 = params.p3 = params.p4 = 1.0

#     order = list(range(len(requests)))

#     # --- сравниваем ---
#     exact_fn = cm.solve_crew_routing
#     ga_fn = cm.solve_crew_routing_ga  # если так назвал

#     args = (requests, workers, tau, roads, params, order, False)

#     exact_times = bench(exact_fn, args, repeats=10, warmup=2)
#     ga_times = bench(ga_fn, args, repeats=10, warmup=2)

#     def report(name, times):
#         print(f"{name}:")
#         print(f"  runs={len(times)}")
#         print(f"  mean={stats.mean(times):.6f}s")
#         print(f"  median={stats.median(times):.6f}s")
#         print(f"  min={min(times):.6f}s  max={max(times):.6f}s")
#         print(f"  stdev={stats.pstdev(times):.6f}s")

#     report("EXACT", exact_times)
#     report("GA", ga_times)

# if __name__ == "__main__":
#     main()