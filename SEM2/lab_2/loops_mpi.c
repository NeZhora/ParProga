#include <mpi.h>

#include "loops.h"

typedef struct {
    int row, col, steps, offset;
    double seed;
} Chain;

static void *allocate(size_t count, size_t size)
{
    void *p = calloc(count ? count : 1, size);
    if (!p) {
        fputs("Cannot allocate MPI buffers\n", stderr);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        exit(EXIT_FAILURE);
    }
    return p;
}

/* Start at an unchanged cell in the first four rows or the last two columns. */
static Chain *enumerate_chains(int rows, int cols, int *count)
{
    *count = 0;
    if (rows <= 4 || cols <= 2)
        return NULL;
    int top = rows - 4 < 4 ? rows - 4 : 4;
    int side = cols - 2 < 2 ? cols - 2 : 2;
    int capacity = top * (cols - 2) + (rows - 4 - top) * side;
    Chain *chains = allocate((size_t)capacity, sizeof *chains);
    int offset = 0;
    for (int i = 0; i < rows - 4; ++i) {
        int first_col = i < 4 ? 2 : cols - side;
        for (int j = first_col; j < cols; ++j) {
            int down = (rows - 1 - i) / 4;
            int left = j / 2;
            int steps = down < left ? down : left;
            chains[(*count)++] = (Chain){i, j, steps, offset, initial_value(i, j)};
            offset += steps;
        }
    }
    return chains;
}

static int first_chain_at(const Chain *chains, int count, int offset)
{
    int low = 0, high = count;
    while (low < high) {
        int mid = low + (high - low) / 2;
        if (chains[mid].offset < offset)
            low = mid + 1;
        else
            high = mid;
    }
    return low;
}

static void run_chains(const Chain *chains, int first, int end, double *result)
{
    for (int c = first; c < end; ++c) {
        double value = chains[c].seed;
        for (int k = 0; k < chains[c].steps; ++k) {
            value = sin(6.0 * value);
            *result++ = value;
        }
    }
}

static void unpack_chains(const Chain *chains, int count, const double *values,
                          double *grid, int cols)
{
    for (int c = 0; c < count; ++c) {
        int i = chains[c].row, j = chains[c].col;
        for (int k = 0; k < chains[c].steps; ++k) {
            i += 4;
            j -= 2;
            grid[(size_t)i * cols + j] = *values++;
        }
    }
}

static void run_rows(int variant, double *a, double *b, double *halo,
                     int rows, int cols, int rank, int active)
{
    const double factor = variant == REFERENCE ? 2.0 : 0.1;
    for (int i = 0; i < rows; ++i)
        sine_row(a + (size_t)i * cols, cols, factor);

    if (variant != THREE_ZH || !rows || cols <= 2)
        return;

    const int prev = rank > 0 ? rank - 1 : MPI_PROC_NULL;
    const int next = rank + 1 < active ? rank + 1 : MPI_PROC_NULL;
    if (active > 1) {
        MPI_Sendrecv(a, cols - 2, MPI_DOUBLE, prev, 0,
                     halo, cols - 2, MPI_DOUBLE, next, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    for (int i = 0; i < rows - 1; ++i)
        produce_3zh_row(b + (size_t)i * cols, a + (size_t)(i + 1) * cols, cols);
    if (next != MPI_PROC_NULL)
        produce_3zh_row(b + (size_t)(rows - 1) * cols, halo, cols);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int rank, workers;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &workers);

    int variant = argc > 1 ? variant_id(argv[1]) : UNKNOWN;
    int rows = argc > 2 ? parse_positive(argv[2], INT_MAX) : 0;
    int cols = argc > 3 ? parse_positive(argv[3], INT_MAX) : 0;
    int repeats = argc > 4 ? parse_positive(argv[4], 1000) : 0;
    if (argc < 5 || argc > 6 || !valid_grid(rows, cols) || !repeats ||
        (variant != REFERENCE && variant != ONE_ZH && variant != THREE_ZH)) {
        if (!rank)
            fprintf(stderr, "Usage: %s ref|1zh|3zh ROWS COLS REPEATS [RESULT.txt]\n", argv[0]);
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    const int chain_mode = variant == ONE_ZH;
    const int total = chain_mode ? (rows > 4 && cols > 2 ? (rows - 4) * (cols - 2) : 0)
                                 : rows * cols;
    int chain_count = 0;
    Chain *chains = chain_mode ? enumerate_chains(rows, cols, &chain_count) : NULL;
    int *counts = allocate((size_t)workers, sizeof *counts);
    int *displacements = allocate((size_t)workers, sizeof *displacements);
    int first_chain = 0, end_chain = 0, displacement = 0;
    for (int p = 0; p < workers; ++p) {
        displacements[p] = displacement;
        if (chain_mode) {
            /* Move each cut to a chain boundary so no dependency crosses ranks. */
            int target = (int)((int64_t)total * (p + 1) / workers);
            int cut = first_chain_at(chains, chain_count, target);
            if (p == rank) {
                first_chain = first_chain_at(chains, chain_count, displacement);
                end_chain = cut;
            }
            displacement = cut < chain_count ? chains[cut].offset : total;
        } else {
            displacement += (rows / workers + (p < rows % workers)) * cols;
        }
        counts[p] = displacement - displacements[p];
    }

    int local_rows = chain_mode ? 0 : counts[rank] / cols;
    int active = workers < rows ? workers : rows;
    double *a = allocate((size_t)counts[rank], sizeof *a);
    double *b = variant == THREE_ZH ? allocate((size_t)counts[rank], sizeof *b) : NULL;
    double *halo = b && local_rows && cols > 2 ? allocate((size_t)cols - 2, sizeof *halo) : NULL;
    double *samples = rank == 0 ? allocate((size_t)repeats, sizeof *samples) : NULL;
    for (int rep = 0; rep < repeats; ++rep) {
        if (!chain_mode)
            fill_rows(a, displacements[rank] / cols, local_rows, cols);
        MPI_Barrier(MPI_COMM_WORLD);
        double start = MPI_Wtime();
        if (chain_mode)
            run_chains(chains, first_chain, end_chain, a);
        else
            run_rows(variant, a, b, halo, local_rows, cols, rank, active);
        double local = MPI_Wtime() - start, maximum;
        MPI_Reduce(&local, &maximum, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        if (!rank)
            samples[rep] = maximum;
    }

    int ok = 1;
    if (argc == 6) {
        double *all = rank == 0 ? allocate((size_t)total, sizeof *all) : NULL;
        MPI_Gatherv(b ? b : a, counts[rank], MPI_DOUBLE, all, counts, displacements,
                    MPI_DOUBLE, 0, MPI_COMM_WORLD);
        if (!rank) {
            double *grid = all;
            if (chain_mode) {
                grid = allocate((size_t)rows * cols, sizeof *grid);
                fill_rows(grid, 0, rows, cols);
                unpack_chains(chains, chain_count, all, grid, cols);
            }
            ok = write_grid(argv[5], grid, rows, cols);
            if (chain_mode)
                free(grid);
            free(all);
        }
        MPI_Bcast(&ok, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
    if (!rank && ok)
        print_timing(variant, "mpi", rows, cols, workers, median_seconds(samples, repeats));

    free(samples);
    free(halo);
    free(b);
    free(a);
    free(displacements);
    free(counts);
    free(chains);
    MPI_Finalize();
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
