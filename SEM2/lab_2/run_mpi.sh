#!/bin/sh
#SBATCH -N 2
#SBATCH --ntasks-per-node=8
#SBATCH --cpus-per-task=1
#SBATCH --time=00:09:50
#SBATCH -o loops-mpi-%j.out
#SBATCH -e loops-mpi-%j.err

set -eu
rows=${ROWS:-5000}
cols=${COLS:-5000}
repeats=${REPEATS:-5}
check_rows=${CHECK_ROWS:-77}
check_cols=${CHECK_COLS:-83}
max_processes=${SLURM_NTASKS:-16}
launcher=${MPIEXEC:-mpirun}
results=results-mpi-${SLURM_JOB_ID:-local}
mkdir -p "$results"

printf '%s\n' 'variant,backend,rows,cols,workers,seconds'
for variant in ref 1zh 3zh; do
    reference="$results/$variant-seq.txt"
    ./loops "$variant" seq "$check_rows" "$check_cols" 1 "$reference" >/dev/null
    ./loops "$variant" seq "$rows" "$cols" "$repeats"
    for processes in 1 2 4 8 16; do
        [ "$processes" -le "$max_processes" ] || continue
        output="$results/$variant-mpi-$processes.txt"
        "$launcher" -n "$processes" ./loops_mpi "$variant" "$check_rows" "$check_cols" 1 "$output" >/dev/null
        cmp "$reference" "$output" >&2
        "$launcher" -n "$processes" ./loops_mpi "$variant" "$rows" "$cols" "$repeats"
    done
done
