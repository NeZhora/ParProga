#include <errno.h>
#include <limits.h>
#include <math.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

typedef struct {
    const double *data;
    int stride;
} Matrix;

enum { NAIVE, TRANSPOSED, STRASSEN, TRANSPOSED_OMP, STRASSEN_OMP };

static const char *const method_names[] = {
    "naive", "transposed", "strassen", "transposed_omp", "strassen_omp"
};

static void *allocate(size_t count, size_t size)
{
    void *buffer = count <= SIZE_MAX / size ? malloc(count * size) : NULL;
    if (!buffer) {
        fputs("Cannot allocate matrix or workspace\n", stderr);
        exit(EXIT_FAILURE);
    }
    return buffer;
}

static int parse_positive(const char *arg, int limit)
{
    char *end;
    errno = 0;
    long value = strtol(arg, &end, 10);
    return !errno && end != arg && !*end && value > 0 && value <= limit
        ? (int)value : 0;
}

static void fill_inputs(double *a, double *b, size_t count, int ones)
{
    uint32_t state = 0x539cb1dfu;
    for (size_t i = 0; i < count; ++i) {
        state = state * 1664525u + 1013904223u;
        a[i] = ones ? 1.0 : (int)((state >> 16) % 17) / 8.0 - 1.0;
        state = state * 1664525u + 1013904223u;
        b[i] = ones ? 1.0 : (int)((state >> 16) % 17) / 8.0 - 1.0;
    }
}

static void naive(const double *a, const double *b, double *c, int n)
{
    for (int i = 0; i < n; ++i) {
        const double *row = a + (size_t)i * n;
        for (int j = 0; j < n; ++j) {
            double sum = 0;
            for (int k = 0; k < n; ++k)
                sum += row[k] * b[(size_t)k * n + j];
            c[(size_t)i * n + j] = sum;
        }
    }
}

static void transpose_row(Matrix b, double *bt, int n, int i)
{
    for (int j = 0; j < n; ++j)
        bt[(size_t)i * n + j] = b.data[(size_t)j * b.stride + i];
}

static void multiply_row(Matrix a, const double *bt, double *c,
                         int stride, int n, int i)
{
    const double *row = a.data + (size_t)i * a.stride;
    for (int j = 0; j < n; ++j) {
        const double *column = bt + (size_t)j * n;
        double sum = 0;
        for (int k = 0; k < n; ++k)
            sum += row[k] * column[k];
        c[(size_t)i * stride + j] = sum;
    }
}

static void transposed(Matrix a, Matrix b, double *c, int stride, int n,
                       double *bt)
{
    for (int i = 0; i < n; ++i)
        transpose_row(b, bt, n, i);
    for (int i = 0; i < n; ++i)
        multiply_row(a, bt, c, stride, n, i);
}

static void transposed_omp(Matrix a, Matrix b, double *c, int stride, int n,
                           double *bt)
{
    #pragma omp parallel
    {
        #pragma omp for schedule(static)
        for (int i = 0; i < n; ++i)
            transpose_row(b, bt, n, i);
        #pragma omp for schedule(static)
        for (int i = 0; i < n; ++i)
            multiply_row(a, bt, c, stride, n, i);
    }
}

typedef struct {
    int first, second, sign;
} Operand;

typedef struct {
    Operand a, b;
    int output_signs[4];
} Product;

/* Quadrants are numbered 11, 12, 21, 22. A sign of zero uses one quadrant. */
static const Product products[] = {
    {{0, 3,  1}, {0, 3,  1}, { 1, 0, 0, 1}},
    {{2, 3,  1}, {0, 0,  0}, { 0, 0, 1,-1}},
    {{0, 0,  0}, {1, 3, -1}, { 0, 1, 0, 1}},
    {{3, 0,  0}, {2, 0, -1}, { 1, 0, 1, 0}},
    {{0, 1,  1}, {3, 0,  0}, {-1, 1, 0, 0}},
    {{2, 0, -1}, {0, 1,  1}, { 0, 0, 0, 1}},
    {{1, 3, -1}, {2, 3,  1}, { 1, 0, 0, 0}}
};

enum { PRODUCT_COUNT = sizeof products / sizeof products[0] };

static size_t offset(int quadrant, int stride, int half)
{
    return (size_t)(quadrant / 2) * half * stride + (quadrant % 2) * half;
}

static Matrix make_operand(Matrix m, Operand op, int half, double *buffer)
{
    const double *first = m.data + offset(op.first, m.stride, half);
    if (!op.sign)
        return (Matrix){first, m.stride};

    const double *second = m.data + offset(op.second, m.stride, half);
    for (int i = 0; i < half; ++i) {
        for (int j = 0; j < half; ++j) {
            size_t pos = (size_t)i * m.stride + j;
            buffer[(size_t)i * half + j] = first[pos] + op.sign * second[pos];
        }
    }
    return (Matrix){buffer, half};
}

static void add_product(double *c, int stride, const double *p, int half,
                        const Product *product)
{
    for (int q = 0; q < 4; ++q) {
        const int sign = product->output_signs[q];
        if (!sign)
            continue;
        double *quadrant = c + offset(q, stride, half);
        for (int i = 0; i < half; ++i) {
            for (int j = 0; j < half; ++j)
                quadrant[(size_t)i * stride + j] += sign * p[(size_t)i * half + j];
        }
    }
}

static void clear_matrix(double *c, int stride, int n)
{
    for (int i = 0; i < n; ++i)
        memset(c + (size_t)i * stride, 0, (size_t)n * sizeof *c);
}

static size_t workspace_size(int n, int cutoff)
{
    size_t total = 0;
    while (n > cutoff) {
        n /= 2;
        total += 3 * (size_t)n * n;
    }
    return total + (size_t)n * n;
}

static void strassen(Matrix a, Matrix b, double *c, int stride, int n,
                     int cutoff, double *work)
{
    if (n <= cutoff) {
        transposed(a, b, c, stride, n, work);
        return;
    }

    const int half = n / 2;
    const size_t area = (size_t)half * half;
    double *p = work + 2 * area;
    clear_matrix(c, stride, n);
    for (int k = 0; k < PRODUCT_COUNT; ++k) {
        const Product *product = &products[k];
        Matrix left = make_operand(a, product->a, half, work);
        Matrix right = make_operand(b, product->b, half, work + area);
        strassen(left, right, p, half, half, cutoff, work + 3 * area);
        add_product(c, stride, p, half, product);
    }
}

static size_t branch_workspace_size(int n, int cutoff)
{
    int half = n / 2;
    return 3 * (size_t)half * half + workspace_size(half, cutoff);
}

static void strassen_omp(Matrix a, Matrix b, double *c, int n, int cutoff,
                         double *work)
{
    if (n <= cutoff) {
        transposed_omp(a, b, c, n, n, work);
        return;
    }

    const int half = n / 2;
    const size_t area = (size_t)half * half;
    const size_t branch = branch_workspace_size(n, cutoff);
    #pragma omp parallel for schedule(static, 1)
    for (int k = 0; k < PRODUCT_COUNT; ++k) {
        double *scratch = work + k * branch;
        Matrix left = make_operand(a, products[k].a, half, scratch);
        Matrix right = make_operand(b, products[k].b, half, scratch + area);
        strassen(left, right, scratch + 2 * area, half, half, cutoff,
                 scratch + 3 * area);
    }

    /* Parallel assembly touches disjoint output rows; each sum uses the same P order. */
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < n; ++i) {
        int top = i < half ? 0 : 2;
        size_t row = (size_t)(i % half) * half;
        for (int j = 0; j < n; ++j) {
            int q = top + (j >= half);
            size_t pos = row + (j % half);
            double sum = 0;
            for (int k = 0; k < PRODUCT_COUNT; ++k) {
                int sign = products[k].output_signs[q];
                if (sign)
                    sum += sign * work[k * branch + 2 * area + pos];
            }
            c[(size_t)i * n + j] = sum;
        }
    }
}

static void pad_input(const double *source, double *target, int n, int padded)
{
    clear_matrix(target, padded, padded);
    for (int i = 0; i < n; ++i)
        memcpy(target + (size_t)i * padded, source + (size_t)i * n,
               (size_t)n * sizeof *target);
}

typedef struct {
    const double *a, *b;
    double *c, *a_pad, *b_pad, *work;
    int n, padded, cutoff;
} Benchmark;

static void run_method(const Benchmark *bench, int method, double *output)
{
    int n = bench->n;
    Matrix a = {bench->a, n}, b = {bench->b, n};
    if (method == NAIVE) {
        naive(a.data, b.data, output, n);
    } else if (method == TRANSPOSED) {
        transposed(a, b, output, n, n, bench->work);
    } else if (method == TRANSPOSED_OMP) {
        transposed_omp(a, b, output, n, n, bench->work);
    } else {
        const int padded = bench->padded;
        if (padded != n) {
            pad_input(a.data, bench->a_pad, n, padded);
            pad_input(b.data, bench->b_pad, n, padded);
            a = (Matrix){bench->a_pad, padded};
            b = (Matrix){bench->b_pad, padded};
        }

        double *c = padded == n ? output : bench->c;
        if (method == STRASSEN_OMP)
            strassen_omp(a, b, c, padded, bench->cutoff, bench->work);
        else
            strassen(a, b, c, padded, padded, bench->cutoff, bench->work);

        if (padded != n) {
            for (int i = 0; i < n; ++i)
                memcpy(output + (size_t)i * n, c + (size_t)i * padded,
                       (size_t)n * sizeof *output);
        }
    }
}

static void verify(const double *result, const double *expected, int n,
                   int ones, int method)
{
    if (!expected && !ones)
        return;
    for (size_t i = 0; i < (size_t)n * n; ++i) {
        double value = expected ? expected[i] : (double)n;
        if (!isfinite(result[i]) || fabs(result[i] - value) > 1e-10 * (1 + fabs(value))) {
            fprintf(stderr, "%s mismatch at (%zu,%zu): %.17g vs %.17g\n",
                    method_names[method], i / n, i % n, result[i], value);
            exit(EXIT_FAILURE);
        }
    }
}

static int compare_seconds(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static double wall_time(void)
{
#ifdef _WIN32
    /* MinGW's OpenMP timer can round small products to zero milliseconds. */
    static LARGE_INTEGER frequency;
    LARGE_INTEGER now;
    if (!frequency.QuadPart)
        QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart / frequency.QuadPart;
#else
    return omp_get_wtime();
#endif
}

static void measure(const Benchmark *bench, int method, double *result,
                    const double *expected, int ones, int threads,
                    int repeats, double *seconds)
{
    for (int rep = 0; rep < repeats; ++rep) {
        double start = wall_time();
        run_method(bench, method, result);
        seconds[rep] = wall_time() - start;
        verify(result, expected, bench->n, ones, method);
    }
    qsort(seconds, (size_t)repeats, sizeof *seconds, compare_seconds);
    double median = (seconds[(repeats - 1) / 2] + seconds[repeats / 2]) / 2;
    printf("%d,%s,%d,%.12g\n", bench->n, method_names[method], threads, median);
}

int main(int argc, char **argv)
{
    int n = argc > 1 ? parse_positive(argv[1], 8192) : 0;
    int repeats = argc > 2 ? parse_positive(argv[2], 1000) : 0;
    int max_threads = argc > 3 ? parse_positive(argv[3], 256) : 0;
    int cutoff = argc > 4 ? parse_positive(argv[4], INT_MAX) : 64;
    int ones = argc == 6 && !strcmp(argv[5], "ones");
    if (!n || !repeats || !max_threads || !cutoff || (argc >= 6 && !ones)) {
        fprintf(stderr, "Usage: %s N REPEATS MAX_THREADS [CUTOFF [ones]]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int padded = 1;
    while (padded < n)
        padded *= 2;
    size_t area = (size_t)n * n, padded_area = (size_t)padded * padded;
    size_t scratch = padded > cutoff
        ? PRODUCT_COUNT * branch_workspace_size(padded, cutoff) : padded_area;
    double *a = allocate(area, sizeof *a), *b = allocate(area, sizeof *b);
    double *reference = allocate(area, sizeof *reference);
    double *result = allocate(area, sizeof *result);
    double *seconds = allocate((size_t)repeats, sizeof *seconds);
    Benchmark bench = {a, b, NULL, NULL, NULL, allocate(scratch, sizeof(double)),
                       n, padded, cutoff};
    if (padded != n) {
        bench.a_pad = allocate(padded_area, sizeof(double));
        bench.b_pad = allocate(padded_area, sizeof(double));
        bench.c = allocate(padded_area, sizeof(double));
    }
    fill_inputs(a, b, area, ones);
    omp_set_dynamic(0);

    measure(&bench, NAIVE, reference, NULL, ones, 1, repeats, seconds);
    measure(&bench, TRANSPOSED, result, reference, ones, 1, repeats, seconds);
    measure(&bench, STRASSEN, result, reference, ones, 1, repeats, seconds);
    for (int threads = 1;;) {
        int actual = 0;
        omp_set_num_threads(threads);
        #pragma omp parallel
        {
            #pragma omp single
            actual = omp_get_num_threads();
        }
        if (actual != threads) {
            fprintf(stderr, "OpenMP supplied %d threads, requested %d\n", actual, threads);
            return EXIT_FAILURE;
        }
        measure(&bench, TRANSPOSED_OMP, result, reference, ones, threads, repeats, seconds);
        measure(&bench, STRASSEN_OMP, result, reference, ones, threads, repeats, seconds);
        if (threads == max_threads)
            break;
        threads = threads <= max_threads / 2 ? 2 * threads : max_threads;
    }

    free(bench.c);
    free(bench.b_pad);
    free(bench.a_pad);
    free(bench.work);
    free(seconds);
    free(result);
    free(reference);
    free(b);
    free(a);
    return EXIT_SUCCESS;
}
