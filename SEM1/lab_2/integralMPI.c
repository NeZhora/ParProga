#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define EXACT_VALUE 10.74678802120157248790729171649
                    

double f(double x) { return cos(1.0 / (x * x)); }

// Адаптивная трапецийная схема. Шаг подбирается автоматически
// до достижения локальной точности eps.
double integrate(double a, double b, double eps) {
    int n = 64;
    double h = (b - a) / n;
    double S = (f(a) + f(b)) * 0.5;
    for (int i = 1; i < n; i++) S += f(a + i * h);
    S *= h;
    double S_old = S;
    while (n < 50000000) {
        n *= 2; h *= 0.5;
        double S_new = S_old * 0.5;
        for (int i = 1; i < n; i += 2) S_new += f(a + i * h) * h;
        if (fabs(S_new - S_old) < eps) return S_new;
        S_old = S_new;
    }
    return S_old;
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0) fprintf(stderr, "Usage: mpirun -np <N> ./prog <eps>\n");
        MPI_Finalize(); return 1;
    }

    double eps = atof(argv[1]);
    double a = 0.04, b = 12.0;

    // Разбиение интервала между процессами
    double local_a = a + rank * (b - a) / size;
    double local_b = a + (rank + 1) * (b - a) / size;

    MPI_Barrier(MPI_COMM_WORLD);
    double t_start = MPI_Wtime();
    // Каждый процесс выбирает свой шаг, стремясь к eps/size
    double local_res = integrate(local_a, local_b, eps / size);
    double t_local = MPI_Wtime() - t_start;

    // Суммирование результатов и замер времени самого медленного процесса
    double global_res = 0.0, max_time = 0.0;
    MPI_Reduce(&local_res, &global_res, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&t_local, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double abs_err = fabs(global_res - EXACT_VALUE);
        
        printf("Результаты:\n");
        printf("  I = : %.10f\n", global_res);
        printf("  Ошибка: %e\n", abs_err);
        printf("  Время вычисления:   %f сек\n", max_time);
        printf("  Процессов:          %d\n", size);
    }

    MPI_Finalize();
    return 0;
}