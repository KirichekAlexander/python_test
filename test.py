import calc_module as cm
import matplotlib.pyplot as plt

# p = [119.36, 123.86, 150.51, 162.69, 160.00, 158.05, 161.61, 149.99, 195.13, 188.69, 146.61, 143.46]  
# V0 = 128.81
# minV = 55.0
# maxV = 220.0

# # запуск итерационного метода
# # res = cm.solve_uniform_pg(p, V0, minV, maxV)
# # dx = 0.0
# # print("ok:", res.ok)
# # print("Mp:", res.Mp)
# # print("x :", res.x)
# # print("V :", res.V)
# # print("maxIter:", res.maxIter)
# # print("iters:", res.iters)
# # 


# # запуск прямого метода
# res = cm.solve_direct(p, V0, minV, maxV)
# dx = 0.08
# print("ok:", res.ok)
# print("x :", res.x)
# print("V :", res.V)
# # 

# x = list(res.x)
# V = list(res.V)

# T = len(p)
# t = list(range(1, T + 1)) 


# minV_line = [minV] * T
# maxV_line = [maxV] * T

# plt.figure(figsize=(10, 6))

# t_minus = [ti - dx for ti in t]
# t_plus  = [ti + dx for ti in t]

# lp, = plt.plot(t, p, linewidth=1.5, label='p')
# lx, = plt.plot(t, x, linewidth=1.5, label='x')
# lV, = plt.plot(t, V, linewidth=1.5, label='V')
# lmin, = plt.plot(t, minV_line, linewidth=1.2, label='minV')
# lmax, = plt.plot(t, maxV_line, linewidth=1.2, label='maxV')

# plt.plot(t_minus, p, marker='s', linestyle='None', fillstyle='none', color=lp.get_color())
# plt.plot(t_plus, x, marker='s', linestyle='None', fillstyle='none', color=lx.get_color())

# plt.plot(t_minus, V, marker='s', linestyle='None', fillstyle='none', color=lV.get_color())
# plt.plot(t_plus, minV_line, marker='s', linestyle='None', fillstyle='none', color=lmin.get_color())

# plt.plot(t, maxV_line, marker='s', linestyle='None', fillstyle='none', color=lmax.get_color())

# plt.xlabel('t')
# plt.ylabel('p, x, V, minV, maxV')
# plt.xticks(t)
# plt.grid(True, alpha=0.3)
# plt.legend(loc='lower left', frameon=True)
# plt.tight_layout()
# plt.show()


# --- Данные из постановки ---
N = 10
M = 6

dur = [1, 3, 5, 2, 2, 1, 1, 3, 5, 2]
rel = [1, 1, 1, 1, 4, 4, 4, 4, 4, 4]
cap = [1, 1, 1, 1, 1, 1]

# demands: список для каждой работы: [(resource_index, qty), ...]
# Матрица Vnm на картинке => у каждой работы ровно один ресурс с qty=1
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

# preds[j] = список предшественников работы j (0-based)
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

N = 10  # или len(dur)
starts = S.start
finishes = S.finish

fig, ax = plt.subplots(figsize=(12, 7))

# --- Гант (полоски) ---
for j in range(N):
    ax.barh(y=j, width=finishes[j] - starts[j], left=starts[j])
    ax.text(finishes[j], j, f" {j+1}", va="center")

ax.set_yticks(range(N))
ax.set_yticklabels([f"Job {j+1}" for j in range(N)])
ax.invert_yaxis()
ax.set_xlabel("Time")
ax.set_title(f"Gantt chart with precedence arrows (Cmax={S.cmax})")

# --- Стрелки предшествований ---
# p -> j: из (finish[p], p) в (start[j], j)
for j in range(N):
    for p in preds[j]:
        x0, y0 = finishes[p], p
        x1, y1 = starts[j], j

        # небольшие смещения, чтобы стрелка не упиралась ровно в край бара
        eps = 0.05
        x0 = x0 - eps
        x1 = x1 + eps

        ax.annotate(
            "", xy=(x1, y1), xytext=(x0, y0),
            arrowprops=dict(arrowstyle="->", linewidth=1),
        )

plt.tight_layout()
plt.show()