import rhythmic_delivery as rd
import matplotlib.pyplot as plt

p = [119.36, 123.86, 150.51, 162.69, 160.00, 158.05, 161.61, 149.99, 195.13, 188.69, 146.61, 143.46]  
V0 = 128.81
minV = 55.0
maxV = 220.0
res = rd.solve(p, V0, minV, maxV)

print("ok:", res.ok)
print("Mp:", res.Mp)
print("x :", res.x)
print("V :", res.V)
x = list(res.x)
V = list(res.V)

T = len(p)
t = list(range(1, T + 1)) 


minV_line = [minV] * T
maxV_line = [maxV] * T

plt.figure(figsize=(10, 6))

plt.plot(t, p, marker='s', fillstyle='none', linewidth=1.5, label='p')
plt.plot(t, x, marker='s', fillstyle='none', linewidth=1.5, label='x')
plt.plot(t, V, marker='s', fillstyle='none', linewidth=1.5, label='V')

plt.plot(t, minV_line, marker='s', fillstyle='none', linewidth=1.2, label='minV')
plt.plot(t, maxV_line, marker='s', fillstyle='none', linewidth=1.2, label='maxV')

plt.xlabel('t')
plt.ylabel('p, x, V, minV, maxV')
plt.xticks(t)
plt.grid(True, alpha=0.3)
plt.legend(loc='lower left', frameon=True)
plt.tight_layout()
plt.show()