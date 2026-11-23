#include <omp.h>

#include "loops.h"

static void sequential(int variant, double *a, double *b, int rows, int cols)
{
    switch (variant) {
    case REFERENCE:
        for (int i = 0; i < rows; ++i)
            sine_row(a + (size_t)i * cols, cols, 2.0);
        break;
    case ONE_ZH:
        for (int i = 4; i < rows; ++i) {
            double *row = a + (size_t)i * cols;
            const double *source = a + (size_t)(i - 4) * cols;
            for (int j = 0; j < cols - 2; ++j)
                row[j] = sin(6.0 * source[j + 2]);
        }
        break;
    case TWO_ZH:
        for (int i = 0; i < rows - 2; ++i) {
            double *row = a + (size_t)i * cols;
            const double *source = a + (size_t)(i + 2) * cols;
            for (int j = 3; j < cols; ++j)
                row[j] = sin(0.1 * source[j - 3]);
        }
        break;
    case THREE_ZH:
        for (int i = 0; i < rows; ++i)
            sine_row(a + (size_t)i * cols, cols, 0.1);
        for (int i = 0; i < rows - 1; ++i)
            produce_3zh_row(b + (size_t)i * cols, a + (size_t)(i + 1) * cols, cols);
        break;
    }
}

static void reference_omp(double *a, int rows, int cols)
{
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < rows; ++i)
        sine_row(a + (size_t)i * cols, cols, 2.0);
}

static void two_zh_omp(const double *a, double *b, int rows, int cols)
{
    /* Reads in the original increasing-i loop always see the input value. */
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < rows; ++i) {
        const double *input = a + (size_t)i * cols;
        double *output = b + (size_t)i * cols;
        if (i >= rows - 2 || cols <= 3) {
            memcpy(output, input, (size_t)cols * sizeof *b);
            continue;
        }
        memcpy(output, input, 3 * sizeof *b);
        const double *source = a + (size_t)(i + 2) * cols;
        for (int j = 3; j < cols; ++j)
            output[j] = sin(0.1 * source[j - 3]);
    }
}

int main(int argc, char **argv)
{
    if (argc != 6 && argc != 7) {
        fprintf(stderr, "Usage: %s VARIANT seq|omp ROWS COLS REPEATS [RESULT.txt]\n", argv[0]);
        return EXIT_FAILURE;
    }
    const int variant = variant_id(argv[1]);
    const int parallel = !strcmp(argv[2], "omp");
    const int rows = parse_positive(argv[3], INT_MAX);
    const int cols = parse_positive(argv[4], INT_MAX);
    const int repeats = parse_positive(argv[5], 1000);
    if (variant == UNKNOWN || !valid_grid(rows, cols) || !repeats ||
        (!parallel && strcmp(argv[2], "seq")) ||
        (parallel && variant != REFERENCE && variant != TWO_ZH)) {
        fputs("Expected ref/1zh/2zh/3zh for seq or ref/2zh for omp, and valid dimensions/repeats\n",
              stderr);
        return EXIT_FAILURE;
    }

    const size_t count = (size_t)rows * cols;
    double *a = malloc(count * sizeof *a);
    int second_grid = variant == THREE_ZH || (parallel && variant == TWO_ZH);
    double *b = NULL;
    if (variant == THREE_ZH)
        b = calloc(count, sizeof *b);
    else if (second_grid)
        b = malloc(count * sizeof *b);
    double *seconds = malloc((size_t)repeats * sizeof *seconds);
    int ok = a && seconds && (!second_grid || b);
    if (!ok) {
        fputs("Cannot allocate grid or timing buffer\n", stderr);
        goto cleanup;
    }

    int workers = 1;
    if (parallel) {
        omp_set_dynamic(0);
        #pragma omp parallel
        {
            #pragma omp single
            workers = omp_get_num_threads();
        }
    }
    for (int rep = 0; rep < repeats; ++rep) {
        fill_rows(a, 0, rows, cols);
        double start = omp_get_wtime();
        if (!parallel)
            sequential(variant, a, b, rows, cols);
        else if (variant == REFERENCE)
            reference_omp(a, rows, cols);
        else
            two_zh_omp(a, b, rows, cols);
        seconds[rep] = omp_get_wtime() - start;
    }

    if (argc == 7)
        ok = write_grid(argv[6], b ? b : a, rows, cols);
    if (ok)
        print_timing(variant, argv[2], rows, cols, workers, median_seconds(seconds, repeats));

cleanup:
    free(seconds);
    free(b);
    free(a);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
