#!/bin/sh
#SBATCH -N 1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --time=00:09:50
#SBATCH -o loops-omp-%j.out
#SBATCH -e loops-omp-%j.err

set -eu
rows=${ROWS:-5000}
cols=${COLS:-5000}
repeats=${REPEATS:-5}
check_rows=${CHECK_ROWS:-77}
check_cols=${CHECK_COLS:-83}
max_threads=${SLURM_CPUS_PER_TASK:-8}
results=results-omp-${SLURM_JOB_ID:-local}
export OMP_PROC_BIND=${OMP_PROC_BIND:-close}
export OMP_PLACES=${OMP_PLACES:-cores}
mkdir -p "$results"

printf '%s\n' 'variant,backend,rows,cols,workers,seconds'
for variant in ref 2zh; do
    reference="$results/$variant-seq.txt"
    ./loops "$variant" seq "$check_rows" "$check_cols" 1 "$reference" >/dev/null
    ./loops "$variant" seq "$rows" "$cols" "$repeats"
    for threads in 1 2 4 8; do
        [ "$threads" -le "$max_threads" ] || continue
        output="$results/$variant-omp-$threads.txt"
        OMP_NUM_THREADS=$threads ./loops "$variant" omp "$check_rows" "$check_cols" 1 "$output" >/dev/null
        cmp "$reference" "$output" >&2
        OMP_NUM_THREADS=$threads ./loops "$variant" omp "$rows" "$cols" "$repeats"
    done
done
