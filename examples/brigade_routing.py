import calc_module as cm
from rus_roads_data import get_rus_roads


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


    sol = cm.solve_crew_routing_ga(requests, # заявки
                                   workers,  # сотрудники
                                   tau,      # время на выполнения задач
                                   roads,    # дороги
                                   params,   # параметры метрики 
                                   order,    # порядок выполнения работ
                                   False     # динамическая важность
                                   ) # генетический алгоритм
    sol = cm.solve_crew_routing(requests, workers, tau, roads, params, order, False)    # прямой перебор
    cm.save_crew_routing_solution_csv("out_csv.csv", sol)

    for rs in sol.perRequest:
        print(f"Заявка id={rs.requestId} | выполнима={'да' if rs.feasible else 'нет'}")
        if not rs.feasible:
            print("  (нет решения)\n")
            continue

        print(f"  Бригада (индексы): {vec(list(rs.team))}")
        print(f"  distKm={rs.distKm:.3f} | t2Hours={rs.t2Hours:.3f} | start={rs.startTime:.3f} | finish={rs.finishTime:.3f} | LC={rs.lc:.3f}")
        print("  Назначение:")
        for j in rs.team:
            ops = rs.assignment[j]
            print(f"    worker #{j} ({workers[j].name}, {workers[j].city}): {vec(list(ops))}")
        print()

if __name__ == "__main__":
    main()