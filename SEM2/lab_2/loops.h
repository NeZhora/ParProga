#ifndef LOOPS_H
#define LOOPS_H

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { REFERENCE, ONE_ZH, TWO_ZH, THREE_ZH, UNKNOWN };

static const char *const variant_names[] = {"ref", "1zh", "2zh", "3zh"};

static inline int variant_id(const char *name)
{
    for (int id = REFERENCE; id < UNKNOWN; ++id) {
        if (!strcmp(name, variant_names[id]))
            return id;
    }
    return UNKNOWN;
}

static inline int parse_positive(const char *arg, int limit)
{
    char *end;
    errno = 0;
    long value = strtol(arg, &end, 10);
    return !errno && end != arg && !*end && value > 0 && value <= limit
        ? (int)value : 0;
}

static inline int valid_grid(int rows, int cols)
{
    return rows > 0 && cols > 0 && rows <= INT_MAX / cols &&
           (size_t)rows * cols <= SIZE_MAX / sizeof(double);
}

static inline double initial_value(int row, int col)
{
    return 10.0 * row + col;
}

static inline void fill_rows(double *a, int first, int rows, int cols)
{
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j)
            a[(size_t)i * cols + j] = initial_value(first + i, j);
    }
}

static inline void sine_row(double *row, int cols, double factor)
{
    for (int j = 0; j < cols; ++j)
        row[j] = sin(factor * row[j]);
}

static inline void produce_3zh_row(double *row, const double *next, int cols)
{
    for (int j = 2; j < cols; ++j)
        row[j] = next[j - 2] * 1.5;
}

static inline int write_grid(const char *path, const double *a, int rows, int cols)
{
    FILE *out = fopen(path, "w");
    if (!out) {
        perror(path);
        return 0;
    }
    for (int i = 0; i < rows && !ferror(out); ++i) {
        for (int j = 0; j < cols; ++j)
            fprintf(out, "%.17g%c", a[(size_t)i * cols + j], j + 1 == cols ? '\n' : ' ');
    }
    int failed = ferror(out);
    if (fclose(out) == EOF || failed) {
        fprintf(stderr, "Could not write %s\n", path);
        return 0;
    }
    return 1;
}

static inline int compare_seconds(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

static inline double median_seconds(double *values, int count)
{
    qsort(values, (size_t)count, sizeof *values, compare_seconds);
    return (values[(count - 1) / 2] + values[count / 2]) / 2;
}

static inline void print_timing(int variant, const char *backend, int rows,
                                int cols, int workers, double seconds)
{
    printf("%s,%s,%d,%d,%d,%.12g\n", variant_names[variant], backend,
           rows, cols, workers, seconds);
}

#endif
