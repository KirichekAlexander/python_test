import random
import calc_module as cm
import matplotlib.pyplot as plt

# параметры теста
N = 50
M = 6
seed_gen = 42

rng = random.Random(seed_gen)

# пусть каждая работа длится от 1 до 6 тактов
dur = [rng.randint(1, 6) for _ in range(N)]

# пусть 70% работ достпны сразу, остальные с задержкой от 1 до 8 тактов
rel = []
for j in range(N):
    rel.append(0 if rng.random() < 0.7 else rng.randint(1, 8))

# объёмы ресурсов доступные в такт от 2 до 4 единиц
cap = [rng.randint(2, 4) for _ in range(M)]

# каждая работа использует от 1 до 2 ресурсов
demands = []
for j in range(N):
    k = 1 if rng.random() < 0.7 else 2
    resources = rng.sample(range(M), k)
    dj = []
    for m in resources:
        qty = rng.randint(1, min(2, cap[m]))
        dj.append((m, qty))
    demands.append(dj)

# preds только i < j
preds = [[] for _ in range(N)]
p_edge = 0.08   # вероятность ребра i->j
max_in = 3      # максимум предшественников у одной работы

for j in range(1, N):
    cand = []
    for i in range(j):
        if rng.random() < p_edge:
            cand.append(i)

    rng.shuffle(cand)
    cand = cand[:max_in]

    # иногда гарантируем хотя бы одного предшественника, чтобы граф не был пустым
    if (not cand) and (rng.random() < 0.35):
        cand = [rng.randint(0, j - 1)]

    preds[j] = sorted(set(cand))

print("Generated instance:")
print("N =", N, "M =", M)
print("cap =", cap)

# запустим решение
S = cm.solve_pcplp(N, M, dur, rel, cap, demands, preds)

print("Cmax =", S.cmax)
print("start =", S.start)
print("finish =", S.finish)

starts = S.start
finishes = S.finish

fig, ax = plt.subplots(figsize=(12, 7))

# диаграмма Ганта
for j in range(N):
    ax.barh(y=j, width=finishes[j] - starts[j], left=starts[j])
    ax.text(finishes[j], j, f" {j+1}", va="center")

ax.set_yticks(range(N))
ax.set_yticklabels([f"Job {j+1}" for j in range(N)])
ax.invert_yaxis()
ax.set_xlabel("Time")
ax.set_title(f"Gantt chart with precedence arrows (Cmax={S.cmax})")

# стрелки от предшественников к последователям
for j in range(N):
    for p in preds[j]:
        x0, y0 = finishes[p], p
        x1, y1 = starts[j], j


        eps = 0.05
        x0 = x0 - eps
        x1 = x1 + eps

        ax.annotate(
            "", xy=(x1, y1), xytext=(x0, y0),
            arrowprops=dict(arrowstyle="->", linewidth=1),
        )

plt.tight_layout()
plt.show()