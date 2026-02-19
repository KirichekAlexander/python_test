import calc_module as cm

def print_vec(v):
    return "[" + ", ".join(str(x) for x in v) + "]"

def main():
    # ---------- 1) Операции ----------
    tau = [2.0, 3.0, 5.0, 4.0, 1.0]  # часы на выполнение операции типа i

    # ---------- 2) Работники ----------
    # Допустим у нас 5 типов поломок, тогда компетенции работника задаются как список, где компетенции индексируются с нуля
    workers = [
        cm.WorkerInput(name="Иван",   city="Москва",      skills=[0, 1, 2]),
        cm.WorkerInput(name="Пётр",   city="Москва",      skills=[1, 3]),
        cm.WorkerInput(name="Ольга",   city="Тверь",       skills=[2, 3, 4]),
        cm.WorkerInput(name="Нина",   city="Тверь",       skills=[0, 4]),
        cm.WorkerInput(name="Сергей", city="Севастополь", skills=[1, 2, 4]),
        cm.WorkerInput(name="Aнна",   city="Севастополь", skills=[0, 3]),
    ]

    # ---------- 3) Заявки ----------
    # Пусть у нас 3 города, где есть объекты, поломки индексируются также с нуля
    requests = [
        cm.RequestInput(id=0, city="Клин",  tasks=[1, 2, 3]),
        cm.RequestInput(id=1, city="Ялта",  tasks=[0, 4]),
        cm.RequestInput(id=2, city="Тверь", tasks=[2, 4]),
    ] # список заявок

    # ---------- 4) Дороги ----------
    roads = [
        cm.RoadEdge(src="Москва", dst="Клин",  km=103.0),
        cm.RoadEdge(src="Клин",   dst="Тверь", km=95.0),
        cm.RoadEdge(src="Москва", dst="Тверь", km=181.0),
        cm.RoadEdge(src="Севастополь", dst="Ялта", km=80.0),
    ] # задаём дороги между городами и расстояние

    # ---------- 5) Параметры ----------
    P = cm.CrewParams()
    P.p1 = 1.0 #
    P.p2 = 1.0 #
               # Веса упущенных компетенций
    P.p3 = 1.0 #
    P.p4 = 1.0 #
    P.speed_kmph = 60.0 # допустим все бригады могут перемещаться с какой-то средней скоростью

    order = [0, 1, 2] # можно задать порядок выполнения заявок

    # ---------- 6) Запуск ----------
    sol = cm.solve_crew_routing(
        requests=requests,  # заявки
        workers=workers,    # сотрудники
        tau=tau,            # время операций
        roads=roads,        # дороги
        params=P,           # параметры модели
        order=order,        # порядок выполнения
        dynamicW=False      # динамическая важность или нет
    )

    # ---------- 7) Печать результата ----------
    for rs in sol.perRequest:
        print(f"Заявка id={rs.requestId} | выполнима={'да' if rs.feasible else 'нет'}")

        if not rs.feasible:
            print("  (нет подходящей бригады)\n")
            continue

        print(f"  Состав бригады (индексы работников): {print_vec(list(rs.team))}")
        print(f"  Расстояние: {rs.distKm:.3f} км | t2 (в одну сторону): {rs.t2Hours:.3f} ч")
        print(f"  Время начала: {rs.startTime:.3f} | Время завершения: {rs.finishTime:.3f}")
        print(f"  LC: {rs.lc:.3f}")
        print("  Распределение операций по работникам:")

        for j in rs.team:
            ops = rs.assignment[j]
            print(f"    Работник #{j} ({workers[j].name}, {workers[j].city}): операции {print_vec(list(ops))}")

        print()

if __name__ == "__main__":
    main()
