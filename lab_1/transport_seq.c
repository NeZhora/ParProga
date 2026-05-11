#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Параметры задачи
#define A_COEFF  1.0
#define X_MAX    1.0
#define T_MAX    1.0

// Вспомогательная функция для замера времени
double get_time_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

// Вспомогательные функции
double phi(double x) { return sin(2.0 * M_PI * x); }
double psi(double t) { return -sin(2.0 * M_PI * A_COEFF * t); }
double f_rhs(double t, double x) { return 0.0; }
double exact_solution(double t, double x) { return sin(2.0 * M_PI * (x - A_COEFF * t)); }

// Схема 1: Явный левый уголок
void scheme_left_corner(int M, int K)
{
    double h = X_MAX / M;
    double tau = T_MAX / K;
    double sigma = A_COEFF * tau / h;

    printf("Схема 1: Явный левый уголок\n");
 
    if (sigma > 1.0 || sigma < 0.0) {
        printf("ПРЕДУПРЕЖДЕНИЕ: условие устойчивости CFL нарушено (sigma = %f)\n", sigma);
    }

    double *u_curr = (double *)malloc((M + 1) * sizeof(double));
    double *u_next = (double *)malloc((M + 1) * sizeof(double));

    for (int m = 0; m <= M; m++) u_curr[m] = phi(m * h);

    // Замер времени: начало
    double t_start = get_time_sec();

    for (int k = 0; k < K; k++) {
        double t_k = k * tau;
        u_next[0] = psi((k + 1) * tau);

        for (int m = 1; m <= M; m++) {
            u_next[m] = u_curr[m] - sigma * (u_curr[m] - u_curr[m - 1]) + tau * f_rhs(t_k, m * h);
        }

        double *tmp = u_curr; u_curr = u_next; u_next = tmp;
    }

    // Замер времени: конец
    double t_end = get_time_sec();
    double elapsed = t_end - t_start;

    double max_err = 0.0, l2_err = 0.0;
    for (int m = 0; m <= M; m++) {
        double exact = exact_solution(T_MAX, m * h);
        double err = fabs(u_curr[m] - exact);
        if (err > max_err) max_err = err;
        l2_err += err * err;
    }
    l2_err = sqrt(l2_err * h);

    printf("Ошибка (max-норма): %e\n", max_err);
    printf("Ошибка (L2-норма):  %e\n", l2_err);
    printf("Время вычислений:   %.6f сек\n", elapsed);

    free(u_curr);
    free(u_next);
}

// Схема 2: Явная четырёхточечная (Лакса-Вендроффа)
void scheme_four_point(int M, int K)
{
    double h = X_MAX / M;
    double tau = T_MAX / K;
    double sigma = A_COEFF * tau / h;

    printf("Схема 2: Явная четырёхточечная (Лакса-Вендроффа)\n");

    if (fabs(sigma) > 1.0) printf("ПРЕДУПРЕЖДЕНИЕ: условие устойчивости нарушено (|sigma| = %f)\n", fabs(sigma));

    double *u_curr = (double *)malloc((M + 1) * sizeof(double));
    double *u_next = (double *)malloc((M + 1) * sizeof(double));

    for (int m = 0; m <= M; m++) u_curr[m] = phi(m * h);

    // Замер времени: начало
    double t_start = get_time_sec();

    for (int k = 0; k < K; k++) {
        double t_k = k * tau;
        u_next[0] = psi((k + 1) * tau);

        for (int m = 1; m < M; m++) {
            u_next[m] = u_curr[m]
                        - (sigma / 2.0) * (u_curr[m + 1] - u_curr[m - 1])
                        + (sigma * sigma / 2.0) * (u_curr[m + 1] - 2.0 * u_curr[m] + u_curr[m - 1])
                        + tau * f_rhs(t_k, m * h);
        }
        u_next[M] = exact_solution((k + 1) * tau, M * h);

        double *tmp = u_curr; u_curr = u_next; u_next = tmp;
    }

    // Замер времени: конец
    double t_end = get_time_sec();
    double elapsed = t_end - t_start;

    double max_err = 0.0, l2_err = 0.0;
    for (int m = 0; m <= M; m++) {
        double err = fabs(u_curr[m] - exact_solution(T_MAX, m * h));
        if (err > max_err) max_err = err;
        l2_err += err * err;
    }
    l2_err = sqrt(l2_err * h);

    printf("Ошибка (max-норма): %e\n", max_err);
    printf("Ошибка (L2-норма):  %e\n", l2_err);
    printf("Время вычислений:   %.6f сек\n", elapsed);

    free(u_curr);
    free(u_next);
}

// Схема 3: Явная центральная трёхточечная (Лакса-Фридрихса)
void scheme_central_three(int M, int K)
{
    double h = X_MAX / M;
    double tau = T_MAX / K;
    double sigma = A_COEFF * tau / h;

    printf("Схема 3: Явная центральная трёхточечная (Лакса-Фридрихса)\n");

    if (fabs(sigma) > 1.0) printf("ПРЕДУПРЕЖДЕНИЕ: условие устойчивости нарушено\n");

    double *u_curr = (double *)malloc((M + 1) * sizeof(double));
    double *u_next = (double *)malloc((M + 1) * sizeof(double));

    for (int m = 0; m <= M; m++) u_curr[m] = phi(m * h);

    // Замер времени: начало
    double t_start = get_time_sec();

    for (int k = 0; k < K; k++) {
        double t_k = k * tau;
        u_next[0] = psi((k + 1) * tau);

        for (int m = 1; m < M; m++) {
            u_next[m] = 0.5 * (u_curr[m + 1] + u_curr[m - 1])
                        - (sigma / 2.0) * (u_curr[m + 1] - u_curr[m - 1])
                        + tau * f_rhs(t_k, m * h);
        }
        u_next[M] = exact_solution((k + 1) * tau, M * h);

        double *tmp = u_curr; u_curr = u_next; u_next = tmp;
    }

    // Замер времени: конец
    double t_end = get_time_sec();
    double elapsed = t_end - t_start;

    double max_err = 0.0, l2_err = 0.0;
    for (int m = 0; m <= M; m++) {
        double err = fabs(u_curr[m] - exact_solution(T_MAX, m * h));
        if (err > max_err) max_err = err;
        l2_err += err * err;
    }
    l2_err = sqrt(l2_err * h);

    printf("Ошибка (max-норма): %e\n", max_err);
    printf("Ошибка (L2-норма):  %e\n", l2_err);
    printf("Время вычислений:   %.6f сек\n", elapsed);

    free(u_curr);
    free(u_next);
}

// Схема 4: Крест (leapfrog)
void scheme_cross(int M, int K)
{
    double h = X_MAX / M;
    double tau = T_MAX / K;
    double sigma = A_COEFF * tau / h;

    printf("Схема 4: Крест (leapfrog)\n");

    if (fabs(sigma) > 1.0) printf("ПРЕДУПРЕЖДЕНИЕ: условие устойчивости нарушено\n");

    double *u_prev = (double *)malloc((M + 1) * sizeof(double));
    double *u_curr = (double *)malloc((M + 1) * sizeof(double));
    double *u_next = (double *)malloc((M + 1) * sizeof(double));

    for (int m = 0; m <= M; m++) u_prev[m] = phi(m * h);

    // Первый шаг (k=0 -> k=1) схемой Лакса-Фридрихса
    u_curr[0] = psi(tau);
    for (int m = 1; m < M; m++) {
        u_curr[m] = 0.5 * (u_prev[m + 1] + u_prev[m - 1])
                    - (sigma / 2.0) * (u_prev[m + 1] - u_prev[m - 1])
                    + tau * f_rhs(0.0, m * h);
    }
    u_curr[M] = exact_solution(tau, M * h);

    // Замер времени: начало
    double t_start = get_time_sec();

    for (int k = 1; k < K; k++) {
        double t_k = k * tau;
        u_next[0] = psi((k + 1) * tau);

        for (int m = 1; m < M; m++) {
            u_next[m] = u_prev[m] - sigma * (u_curr[m + 1] - u_curr[m - 1]) + 2.0 * tau * f_rhs(t_k, m * h);
        }
        u_next[M] = exact_solution((k + 1) * tau, M * h);

        double *tmp = u_prev; u_prev = u_curr; u_curr = u_next; u_next = tmp;
    }

    // Замер времени: конец
    double t_end = get_time_sec();
    double elapsed = t_end - t_start;

    double max_err = 0.0, l2_err = 0.0;
    for (int m = 0; m <= M; m++) {
        double err = fabs(u_curr[m] - exact_solution(T_MAX, m * h));
        if (err > max_err) max_err = err;
        l2_err += err * err;
    }
    l2_err = sqrt(l2_err * h);

    printf("Ошибка (max-норма): %e\n", max_err);
    printf("Ошибка (L2-норма):  %e\n", l2_err);
    printf("Время вычислений:   %.6f сек\n", elapsed);

    free(u_prev);
    free(u_curr);
    free(u_next);
}

// Схема 5: Прямоугольник (box scheme)
void scheme_rectangle(int M, int K)
{
    double h = X_MAX / M;
    double tau = T_MAX / K;
    double sigma = A_COEFF * tau / h;

    printf("Схема 5: Прямоугольник (box scheme)\n");


    double *u_curr = (double *)malloc((M + 1) * sizeof(double));
    double *u_next = (double *)malloc((M + 1) * sizeof(double));

    for (int m = 0; m <= M; m++) u_curr[m] = phi(m * h);

    // Замер времени: начало
    double t_start = get_time_sec();

    for (int k = 0; k < K; k++) {
        double t_k = k * tau;
        double t_kp1 = (k + 1) * tau;

        u_next[0] = psi(t_kp1);

        for (int m = 1; m <= M; m++) {
            double f_val = f_rhs(t_k + 0.5 * tau, (m - 0.5) * h);
            u_next[m] = (u_curr[m] * (1.0 - sigma)
                         + u_curr[m - 1] * (1.0 + sigma)
                         - u_next[m - 1] * (1.0 - sigma)
                         + 2.0 * tau * f_val) / (1.0 + sigma);
        }

        double *tmp = u_curr; u_curr = u_next; u_next = tmp;
    }

    // Замер времени: конец
    double t_end = get_time_sec();
    double elapsed = t_end - t_start;

    double max_err = 0.0, l2_err = 0.0;
    for (int m = 0; m <= M; m++) {
        double err = fabs(u_curr[m] - exact_solution(T_MAX, m * h));
        if (err > max_err) max_err = err;
        l2_err += err * err;
    }
    l2_err = sqrt(l2_err * h);

    printf("Ошибка (max-норма): %e\n", max_err);
    printf("Ошибка (L2-норма):  %e\n", l2_err);
    printf("Время вычислений:   %.6f сек\n", elapsed);

    free(u_curr);
    free(u_next);
}

// Точка входа
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