#include "rhythmic_delivery.h"


// функция проекции вектора на многомерный куб задаваемый векторами lb, ub
void clamp_vec(Vec& vec, Vec const& lb, Vec const& ub) {

    size_t vec_sz = vec.size();
    for (size_t i = 0; i < vec_sz; ++i) {

        if (vec[i] < lb[i]) vec[i] = lb[i];
        if (vec[i] > ub[i]) vec[i] = ub[i];

    }

}



RhythmicResult solve_rhythmic_delivery(Vec const& p, double V0, double minV, double maxV) {

    size_t n = p.size(); // количество тактов поставок

    Vec lb(n, 0.0); // вектор нижних границ
    Vec ub(n, 0.0); // вектор верхних границ

    double s = 0.0; // сумма поставок из РС потребителям
    

    // вычисление суммы поставок, нижних и верхних границ поставок в каждый такт
    for(size_t t = 0; t < n; ++t) {

        s += p[t];             // сумма поставок
        lb[t] = minV - V0 + s; // нижние границы
        ub[t] = maxV - V0 + s; // верхние границы

    }
    double Mp = s / n; // средняя величина поставок
    //


    // метод проекции градиента
    const int maxIter = 200000; // максимальное количество итераций
    const double eps  = 1e-10;  // желаемая точность(критерий остановки)
    double alpha      = 0.125;  // шаг метода


    // код метода
    // сделаем начальное приближение
    Vec y(n, 0.0);
    for (size_t t = 0; t < n; ++t) {

        // y[t] = (t + 1.0) * Mp; // для задачи из статьи идеальное начальное приближение
        y[t] = 0.5*(lb[t]+ub[t]); // приближение средними границ

    }
    clamp_vec(y, lb, ub); // проецирование на множество lb[t]<=y[t]<=ub[t]

    Vec r(n, 0.0);  // r[t] = x[t] - Mp
    Vec g(n, 0.0);  // градиент F(y) grad(F)[t] = g[t] = 2r[t] - 2r[t+1] или 2r[n]
    Vec new_y(n, 0.0); // новое приближение


    // градиентный спуск
    for (int it = 0; it < maxIter; ++it) {

        // вычисление r[t]
        r[0] = y[0] - Mp;
        for (int t = 1; t < n; ++t) {
            r[t] = (y[t] - y[t - 1]) - Mp;
        }
        //

        // вычисления градиента
        for (int i = 0; i < n - 1; ++i) {
            g[i] = 2.0 * r[i] - 2.0 * r[i + 1];
        }
        g[n - 1] = 2.0 * r[n - 1];
        //

        // шаг метода
        new_y = y - alpha * g;
        clamp_vec(new_y, lb, ub); // проецирование нового приближения на допустимое множество
        double maxDiff = lc_norm(new_y - y); // норма разности
        std::swap(y, new_y); // меням местами память

        // критерий остановки
        if (maxDiff < eps) {
            break;
        }

    }
    //

    // восстановление x
    Vec x(n, 0.0);
    x[0] = y[0];
    for (int t = 1; t < n; ++t) {
        x[t] = y[t] - y[t - 1];
    }
    //

    Vec vecV(n, 0.0); // объёмы склада
    bool ok = true;   // флаг того, что границы были соблюдены

    // проверка границ
    for (int t = 0; t < n; ++t) {

        vecV[t] = x[t] - p[t] + (t == 0 ? V0 : vecV[t - 1]);
        if (vecV[t] < minV or vecV[t] > maxV) {
            ok = false;
        }

    }
    //

    return RhythmicResult{x,
                          vecV,
                          Mp,
                          ok};

}