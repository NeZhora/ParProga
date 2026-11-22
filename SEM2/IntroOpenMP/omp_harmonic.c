#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char *argv[])
{
    long n;
    double parts[256];

    if (argc < 2) {
        printf("Usage: %s <N>\n", argv[0]);
        return 1;
    }

    n = atol(argv[1]);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int cnt = omp_get_num_threads();
        long start, end, i;
        double sum = 0.0;

        start = (long)((double)n * tid / cnt) + 1;
        end = (long)((double)n * (tid + 1) / cnt);

        for (i = start; i <= end; i++) {
            sum += 1.0 / (double)i;
        }

        parts[tid] = sum;

        #pragma omp barrier

        #pragma omp master
        {
            double total = 0.0;
            int k;

            for (k = 0; k < cnt; k++) {
                total += parts[k];
            }

            printf("N = %ld\n", n);
            printf("Threads = %d\n", cnt);
            printf("Sum = %.15f\n", total);
        }
    }

    return 0;
}
