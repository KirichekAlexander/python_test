import calc_module as cm
import matplotlib.pyplot as plt

p = [119.36, 123.86, 150.51, 162.69, 160.00, 158.05, 161.61, 149.99, 195.13, 188.69, 146.61, 143.46]  
V0 = 128.81
minV = 55.0
maxV = 220.0

# запуск итерационного метода
res = cm.solve_uniform_pg(p, V0, minV, maxV)
dx = 0.0
print("ok:", res.ok)
print("Mp:", res.Mp)
print("x :", res.x)
print("V :", res.V)
print("maxIter:", res.maxIter)
print("iters:", res.iters)
# 

x = list(res.x)
V = list(res.V)

T = len(p)
t = list(range(1, T + 1)) 


minV_line = [minV] * T
maxV_line = [maxV] * T

plt.figure(figsize=(10, 6))

t_minus = [ti - dx for ti in t]
t_plus  = [ti + dx for ti in t]

lp, = plt.plot(t, p, linewidth=1.5, label='p')
lx, = plt.plot(t, x, linewidth=1.5, label='x')
lV, = plt.plot(t, V, linewidth=1.5, label='V')
lmin, = plt.plot(t, minV_line, linewidth=1.2, label='minV')
lmax, = plt.plot(t, maxV_line, linewidth=1.2, label='maxV')

plt.plot(t_minus, p, marker='s', linestyle='None', fillstyle='none', color=lp.get_color())
plt.plot(t_plus, x, marker='s', linestyle='None', fillstyle='none', color=lx.get_color())

plt.plot(t_minus, V, marker='s', linestyle='None', fillstyle='none', color=lV.get_color())
plt.plot(t_plus, minV_line, marker='s', linestyle='None', fillstyle='none', color=lmin.get_color())

plt.plot(t, maxV_line, marker='s', linestyle='None', fillstyle='none', color=lmax.get_color())

plt.xlabel('t')
plt.ylabel('p, x, V, minV, maxV')
plt.xticks(t)
plt.grid(True, alpha=0.3)
plt.legend(loc='lower left', frameon=True)
plt.tight_layout()
plt.show()