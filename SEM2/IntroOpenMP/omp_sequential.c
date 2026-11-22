#include <stdio.h>
#include <omp.h>

int main()
{
    int value = 0;
    int nthreads = 0;

    #pragma omp parallel
    {
        int cnt = omp_get_num_threads();

        #pragma omp single
        {
            nthreads = cnt;
            printf("Start value = %d, threads = %d\n", value, cnt);
        }

        #pragma omp for ordered schedule(static, 1)
        for (int i = 0; i < nthreads; i++) {
            #pragma omp ordered
            {
                value = value + 1;
                printf("Thread %d got access, value = %d\n",
                       omp_get_thread_num(), value);
            }
        }
    }

    printf("Final value = %d\n", value);

    return 0;
}
