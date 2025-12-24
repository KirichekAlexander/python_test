import calc_module as cm
import matplotlib.pyplot as plt


# Данные из постановки
N = 10
M = 6

dur = [1, 3, 5, 2, 2, 1, 1, 3, 5, 2]
rel = [1, 1, 1, 1, 4, 4, 4, 4, 4, 4]
cap = [1, 1, 1, 1, 1, 1]

# требования работ по ресурсам
demands = [
    [(0, 1)],  # job 1
    [(1, 1)],  # job 2
    [(2, 1)],  # job 3
    [(3, 1)],  # job 4
    [(4, 1)],  # job 5
    [(5, 1)],  # job 6
    [(0, 1)],  # job 7
    [(1, 1)],  # job 8
    [(2, 1)],  # job 9
    [(3, 1)],  # job 10
]

# preds[j] = список предшественников работы j 
preds = [
    [],         # 1
    [0],        # 2  (1->2)
    [0],        # 3  (1->3)
    [1, 2],     # 4  (2->4, 3->4)
    [],         # 5
    [4],        # 6  (5->6)
    [0],        # 7  (1->7)
    [6, 1],     # 8  (7->8, 2->8)
    [6, 2],     # 9  (7->9, 3->9)
    [7, 8, 3],  # 10 (8->10, 9->10, 4->10)
]

S = cm.solve_pcplp(
    N, M, dur, rel, cap, demands, preds
)

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