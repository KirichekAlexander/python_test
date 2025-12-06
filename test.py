import rhythmic_delivery as rd

p = [119.36, 123.86, 150.51, 162.69, 160.00, 158.05, 161.61, 149.99, 195.13, 188.69, 146.61, 143.46]   # параметры/запросы
res = rd.solve(p, 128.81, 55, 220)

print("ok:", res.ok)
print("Mp:", res.Mp)
print("x :", res.x)
print("V :", res.V)
