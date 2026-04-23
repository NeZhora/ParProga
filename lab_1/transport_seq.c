/* transport_seq.c
 *
 * Последовательное численное решение уравнения переноса:
 *   du/dt + a * du/dx = f(t,x),  0 <= t <= T, 0 <= x <= X
 *   u(0,x) = phi(x)   — начальное условие
 *   u(t,0) = psi(t)   — граничное условие
 *
 * Реализованы 5 разностных схем:
 *   1. Явный левый уголок
 *   2. Явная четырёхточечная схема (Лакса-Вендроффа)
 *   3. Явная центральная трёхточечная схема (Лакса-Фридрихса)
 *   4. Схема «крест» (leapfrog)
 *   5. Схема «прямоугольник» (трапеция)
 *
 * Тестовая задача:
 *   a = 1, f(t,x) = 0
 *   phi(x) = sin(2*pi*x)  (начальное условие)
 *   psi(t) = -sin(2*pi*t) (граничное условие, из точного решения u = sin(2*pi*(x-t)))
 *   Точное решение: u(t,x) = sin(2*pi*(x - a*t))
 *
 * Компиляция: gcc -O2 -o transport_seq transport_seq.c -lm
 * Запуск:     ./transport_seq <scheme> <M> <K>
 *   scheme: 1-5 (номер схемы)
 *   M: число узлов по x
 *   K: число узлов по t
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Параметры задачи */
#define A_COEFF  1.0   /* Скорость переноса */
#define X_MAX    1.0   /* Правая граница по x */
#define T_MAX    1.0   /* Правая граница по t */

/* Начальное условие phi(x) */
double phi(double x)
{
    return sin(2.0 * M_PI * x);
}

/* Граничное условие psi(t) */
double psi(double t)
{
    return -sin(2.0 * M_PI * A_COEFF * t);
}

/* Правая часть f(t,x) */
double f_rhs(double t, double x)
{
    return 0.0;  /* Однородное уравнение для тестовой задачи */
}

/* Точное решение для проверки */
double exact_solution(double t, double x)
{
    return sin(2.0 * M_PI * (x - A_COEFF * t));
}

/* ============================================================
 * Схема 1: Явный левый уголок
 * (u_m^{k+1} - u_m^k) / tau + a*(u_m^k - u_{m-1}^k) / h = f_m^k
 *
 * => u_m^{k+1} = u_m^k - a*tau/h * (u_m^k - u_{m-1}^k) + tau*f_m^k
 *
 * Условие устойчивости (CFL): 0 <= a*tau/h <= 1
 * ============================================================ */
void scheme_left_corner(int M, int K)
{
    double h = X_MAX / M;
    double tau = T_MAX / K;
    double sigma = A_COEFF * tau / h;  /* Число Куранта */

    printf("Схема 1: Явный левый уголок\n");
    printf("h = %e, tau = %e, sigma (Куран) = %f\n", h, tau, sigma);

    if (sigma > 1.0 || sigma < 0.0) {
        printf("ПРЕДУПРЕЖДЕНИЕ: условие устойчивости CFL нарушено (sigma = %f)\n", sigma);
    }

    /* Выделяем два слоя по времени: текущий (u_curr) и следующий (u_next) */
    double *u_curr = (double *)malloc((M + 1) * sizeof(double));
    double *u_next = (double *)malloc((M + 1) * sizeof(double));

    /* Начальное условие: u(0, x_m) = phi(x_m) */
    int m;
    for (m = 0; m <= M; m++) {
        u_curr[m] = phi(m * h);
    }

    /* Временной цикл */
    int k;
    for (k = 0; k < K; k++) {
        double t_k = k * tau;

        /* Граничное условие: u(t_{k+1}, 0) = psi(t_{k+1}) */
        u_next[0] = psi((k + 1) * tau);

        /* Внутренние точки */
        for (m = 1; m <= M; m++) {
            u_next[m] = u_curr[m]
                        - sigma * (u_curr[m] - u_curr[m - 1])
                        + tau * f_rhs(t_k, m * h);
        }

        /* Переключаем слои */
        double *tmp = u_curr;
        u_curr = u_next;
        u_next = tmp;
    }

    /* Вычисляем ошибку на последнем временном слое */
    double max_err = 0.0;
    double l2_err = 0.0;
    for (m = 0; m <= M; m++) {
        double exact = exact_solution(T_MAX, m * h);
        double err = fabs(u_curr[m] - exact);
        if (err > max_err) max_err = err;
        l2_err += err * err;
    }
    l2_err = sqrt(l2_err * h);

    printf("Ошибка (max-норма): %e\n", max_err);
    printf("Ошибка (L2-норма):  %e\n", l2_err);

    free(u_curr);
    free(u_next);
}

/* ============================================================
 * Схема 2: Явная четырёхточечная (Лакса-Вендроффа)
 * (u_m^{k+1} - u_m^k)/tau + a*(u_{m+1}^k - u_{m-1}^k)/(2h)
 *   - 0.5*a^2*tau*(u_{m+1}^k - 2*u_m^k + u_{m-1}^k)/h^2 = f_m^k
 *
 * => u_m^{k+1} = u_m^k
 *     - sigma/2 * (u_{m+1}^k - u_{m-1}^k)
 *     + sigma^2/2 * (u_{m+1}^k - 2*u_m^k + u_{m-1}^k)
 *     + tau * f_m^k
 *
 * Условие устойчивости: |sigma| <= 1
 * ============================================================ */
void scheme_four_point(int M, int K)
{
    double h = X_MAX / M;
    double tau = T_MAX / K;
    double sigma = A_COEFF * tau / h;

    printf("Схема 2: Явная четырёхточечная (Лакса-Вендроффа)\n");
    printf("h = %e, tau = %e, sigma = %f\n", h, tau, sigma);

    if (fabs(sigma) > 1.0) {
        printf("ПРЕДУПРЕЖДЕНИЕ: условие устойчивости нарушено (|sigma| = %f)\n", fabs(sigma));
    }

    double *u_curr = (double *)malloc((M + 1) * sizeof(double));
    double *u_next = (double *)malloc((M + 1) * sizeof(double));

    int m;
    for (m = 0; m <= M; m++) {
        u_curr[m] = phi(m * h);
    }

    int k;
    for (k = 0; k < K; k++) {
        double t_k = k * tau;

        u_next[0] = psi((k + 1) * tau);

        for (m = 1; m < M; m++) {
            u_next[m] = u_curr[m]
                        - (sigma / 2.0) * (u_curr[m + 1] - u_curr[m - 1])
                        + (sigma * sigma / 2.0) * (u_curr[m + 1] - 2.0 * u_curr[m] + u_curr[m - 1])
                        + tau * f_rhs(t_k, m * h);
        }

        /* Правая граница: экстраполяция или точное решение.
         * Используем точное решение для простоты тестирования. */
        u_next[M] = exact_solution((k + 1) * tau, M * h);

        double *tmp = u_curr;
        u_curr = u_next;
        u_next = tmp;
    }

    double max_err = 0.0, l2_err = 0.0;
    for (m = 0; m <= M; m++) {
        double err = fabs(u_curr[m] - exact_solution(T_MAX, m * h));
        if (err > max_err) max_err = err;
        l2_err += err * err;
    }
    l2_err = sqrt(l2_err * h);

    printf("Ошибка (max-норма): %e\n", max_err);
    printf("Ошибка (L2-норма):  %e\n", l2_err);

    free(u_curr);
    free(u_next);
}

/* ============================================================
 * Схема 3: Явная центральная трёхточечная (Лакса-Фридрихса)
 * (u_m^{k+1} - 0.5*(u_{m+1}^k + u_{m-1}^k)) / tau
 *   + a*(u_{m+1}^k - u_{m-1}^k) / (2h) = f_m^k
 *
 * => u_m^{k+1} = 0.5*(u_{m+1}^k + u_{m-1}^k)
 *     - sigma/2 * (u_{m+1}^k - u_{m-1}^k)
 *     + tau * f_m^k
 *
 * Условие устойчивости: |sigma| <= 1
 * ============================================================ */
void scheme_central_three(int M, int K)
{
    double h = X_MAX / M;
    double tau = T_MAX / K;
    double sigma = A_COEFF * tau / h;

    printf("Схема 3: Явная центральная трёхточечная (Лакса-Фридрихса)\n");
    printf("h = %e, tau = %e, sigma = %f\n", h, tau, sigma);

    if (fabs(sigma) > 1.0) {
        printf("ПРЕДУПРЕЖДЕНИЕ: условие устойчивости нарушено\n");
    }

    double *u_curr = (double *)malloc((M + 1) * sizeof(double));
    double *u_next = (double *)malloc((M + 1) * sizeof(double));

    int m;
    for (m = 0; m <= M; m++) {
        u_curr[m] = phi(m * h);
    }

    int k;
    for (k = 0; k < K; k++) {
        double t_k = k * tau;

        u_next[0] = psi((k + 1) * tau);

        for (m = 1; m < M; m++) {
            u_next[m] = 0.5 * (u_curr[m + 1] + u_curr[m - 1])
                        - (sigma / 2.0) * (u_curr[m + 1] - u_curr[m - 1])
                        + tau * f_rhs(t_k, m * h);
        }

        u_next[M] = exact_solution((k + 1) * tau, M * h);

        double *tmp = u_curr;
        u_curr = u_next;
        u_next = tmp;
    }

    double max_err = 0.0, l2_err = 0.0;
    for (m = 0; m <= M; m++) {
        double err = fabs(u_curr[m] - exact_solution(T_MAX, m * h));
        if (err > max_err) max_err = err;
        l2_err += err * err;
    }
    l2_err = sqrt(l2_err * h);

    printf("Ошибка (max-норма): %e\n", max_err);
    printf("Ошибка (L2-норма):  %e\n", l2_err);

    free(u_curr);
    free(u_next);
}

/* ============================================================
 * Схема 4: Крест (leapfrog)
 * (u_m^{k+1} - u_m^{k-1}) / (2*tau) + a*(u_{m+1}^k - u_{m-1}^k) / (2h) = f_m^k
 *
 * => u_m^{k+1} = u_m^{k-1} - sigma * (u_{m+1}^k - u_{m-1}^k) + 2*tau*f_m^k
 *
 * Трёхслойная схема: нужны два предыдущих слоя.
 * Первый шаг (k=0 -> k=1) делаем схемой Лакса-Фридрихса.
 *
 * Условие устойчивости: |sigma| <= 1
 * ============================================================ */
void scheme_cross(int M, int K)
{
    double h = X_MAX / M;
    double tau = T_MAX / K;
    double sigma = A_COEFF * tau / h;

    printf("Схема 4: Крест (leapfrog)\n");
    printf("h = %e, tau = %e, sigma = %f\n", h, tau, sigma);

    if (fabs(sigma) > 1.0) {
        printf("ПРЕДУПРЕЖДЕНИЕ: условие устойчивости нарушено\n");
    }

    /* Три слоя: u_prev (k-1), u_curr (k), u_next (k+1) */
    double *u_prev = (double *)malloc((M + 1) * sizeof(double));
    double *u_curr = (double *)malloc((M + 1) * sizeof(double));
    double *u_next = (double *)malloc((M + 1) * sizeof(double));

    int m;
    /* Слой k=0: начальное условие */
    for (m = 0; m <= M; m++) {
        u_prev[m] = phi(m * h);
    }

    /* Слой k=1: используем схему Лакса-Фридрихса для первого шага */
    u_curr[0] = psi(tau);
    for (m = 1; m < M; m++) {
        u_curr[m] = 0.5 * (u_prev[m + 1] + u_prev[m - 1])
                    - (sigma / 2.0) * (u_prev[m + 1] - u_prev[m - 1])
                    + tau * f_rhs(0.0, m * h);
    }
    u_curr[M] = exact_solution(tau, M * h);

    /* Основной цикл: k = 1, 2, ..., K-1 */
    int k;
    for (k = 1; k < K; k++) {
        double t_k = k * tau;

        u_next[0] = psi((k + 1) * tau);

        for (m = 1; m < M; m++) {
            u_next[m] = u_prev[m]
                        - sigma * (u_curr[m + 1] - u_curr[m - 1])
                        + 2.0 * tau * f_rhs(t_k, m * h);
        }

        u_next[M] = exact_solution((k + 1) * tau, M * h);

        /* Сдвигаем слои: prev <- curr, curr <- next */
        double *tmp = u_prev;
        u_prev = u_curr;
        u_curr = u_next;
        u_next = tmp;
    }

    double max_err = 0.0, l2_err = 0.0;
    for (m = 0; m <= M; m++) {
        double err = fabs(u_curr[m] - exact_solution(T_MAX, m * h));
        if (err > max_err) max_err = err;
        l2_err += err * err;
    }
    l2_err = sqrt(l2_err * h);

    printf("Ошибка (max-норма): %e\n", max_err);
    printf("Ошибка (L2-норма):  %e\n", l2_err);

    free(u_prev);
    free(u_curr);
    free(u_next);
}

/* ============================================================
 * Схема 5: Прямоугольник (трапеция / box scheme)
 * (u_{m-1}^{k+1} - u_{m-1}^k + u_m^{k+1} - u_m^k) / (2*tau)
 *   + a*(u_m^{k+1} - u_{m-1}^{k+1} + u_m^k - u_{m-1}^k) / (2*h) = f_{m-1/2}^{k+1/2}
 *
 * Это неявная схема, но она может быть решена явной прогонкой слева направо,
 * т.к. u_m^{k+1} выражается через u_{m-1}^{k+1} (уже известно) и значения на слое k.
 *
 * Перегруппируем:
 * u_m^{k+1} * (1/(2*tau) + a/(2*h)) =
 *   u_m^k * (1/(2*tau) - a/(2*h))
 *   + u_{m-1}^k * (-1/(2*tau) - a/(2*h))  [нет, пересчитаем]
 *
 * Аккуратнее:
 * u_{m-1}^{k+1}/(2*tau) + u_m^{k+1}/(2*tau) + a*u_m^{k+1}/(2h) - a*u_{m-1}^{k+1}/(2h)
 * = u_{m-1}^k/(2*tau) + u_m^k/(2*tau) - a*u_m^k/(2h) + a*u_{m-1}^k/(2h) + f
 *
 * u_m^{k+1} * (1 + sigma) = u_m^k * (1 - sigma)
 *   + u_{m-1}^k * (1 + sigma) - u_{m-1}^{k+1} * (1 - sigma)
 *   + 2*tau * f_{m-1/2}^{k+1/2}
 *
 * Нет, давайте выведем аккуратно. Обозначим sigma = a*tau/h.
 *
 * Из разностной схемы:
 * (u_{m-1}^{k+1} + u_m^{k+1} - u_{m-1}^k - u_m^k) / (2*tau)
 *   + a * (u_m^{k+1} - u_{m-1}^{k+1} + u_m^k - u_{m-1}^k) / (2*h) = f
 *
 * Умножим на 2*tau:
 * (u_{m-1}^{k+1} + u_m^{k+1} - u_{m-1}^k - u_m^k)
 *   + sigma * (u_m^{k+1} - u_{m-1}^{k+1} + u_m^k - u_{m-1}^k) = 2*tau*f
 *
 * u_m^{k+1} * (1 + sigma) + u_{m-1}^{k+1} * (1 - sigma)
 *   = u_m^k * (1 - sigma) + u_{m-1}^k * (1 + sigma) + 2*tau*f
 *
 * => u_m^{k+1} = [ u_m^k*(1-sigma) + u_{m-1}^k*(1+sigma)
 *                   - u_{m-1}^{k+1}*(1-sigma) + 2*tau*f ] / (1+sigma)
 *
 * Прогонка слева направо: u_0^{k+1} = psi(t_{k+1}), затем m = 1, 2, ..., M
 * ============================================================ */
void scheme_rectangle(int M, int K)
{
    double h = X_MAX / M;
    double tau = T_MAX / K;
    double sigma = A_COEFF * tau / h;

    printf("Схема 5: Прямоугольник (box scheme)\n");
    printf("h = %e, tau = %e, sigma = %f\n", h, tau, sigma);

    double *u_curr = (double *)malloc((M + 1) * sizeof(double));
    double *u_next = (double *)malloc((M + 1) * sizeof(double));

    int m;
    for (m = 0; m <= M; m++) {
        u_curr[m] = phi(m * h);
    }

    int k;
    for (k = 0; k < K; k++) {
        double t_k = k * tau;
        double t_kp1 = (k + 1) * tau;

        /* Граничное условие */
        u_next[0] = psi(t_kp1);

        /* Прогонка слева направо */
        for (m = 1; m <= M; m++) {
            /* f в точке (t_{k+1/2}, x_{m-1/2}) */
            double f_val = f_rhs(t_k + 0.5 * tau, (m - 0.5) * h);

            u_next[m] = (u_curr[m] * (1.0 - sigma)
                         + u_curr[m - 1] * (1.0 + sigma)
                         - u_next[m - 1] * (1.0 - sigma)
                         + 2.0 * tau * f_val) / (1.0 + sigma);
        }

        double *tmp = u_curr;
        u_curr = u_next;
        u_next = tmp;
    }

    double max_err = 0.0, l2_err = 0.0;
    for (m = 0; m <= M; m++) {
        double err = fabs(u_curr[m] - exact_solution(T_MAX, m * h));
        if (err > max_err) max_err = err;
        l2_err += err * err;
    }
    l2_err = sqrt(l2_err * h);

    printf("Ошибка (max-норма): %e\n", max_err);
    printf("Ошибка (L2-норма):  %e\n", l2_err);

    free(u_curr);
    free(u_next);
}

/* ============================================================ */
int main(int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr, "Использование: ./transport_seq <scheme> <M> <K>\n");
        fprintf(stderr, "  scheme: 1-5 (номер разностной схемы)\n");
        fprintf(stderr, "  M: число узлов по x\n");
        fprintf(stderr, "  K: число узлов по t\n");
        return 1;
    }

    int scheme = atoi(argv[1]);
    int M = atoi(argv[2]);
    int K = atoi(argv[3]);

    printf("=== Уравнение переноса: du/dt + %.1f * du/dx = 0 ===\n", A_COEFF);
    printf("Область: [0, %.1f] x [0, %.1f]\n", T_MAX, X_MAX);
    printf("Сетка: M = %d (по x), K = %d (по t)\n\n", M, K);

    switch (scheme) {
        case 1: scheme_left_corner(M, K);   break;
        case 2: scheme_four_point(M, K);    break;
        case 3: scheme_central_three(M, K); break;
        case 4: scheme_cross(M, K);         break;
        case 5: scheme_rectangle(M, K);     break;
        default:
            fprintf(stderr, "Неизвестная схема: %d. Допустимые: 1-5.\n", scheme);
            return 1;
    }

    return 0;
}