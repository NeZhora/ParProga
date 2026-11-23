#!/bin/sh
#SBATCH -N 1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=8
#SBATCH --time=00:09:50
#SBATCH -o matmul-%j.out
#SBATCH -e matmul-%j.err

set -eu
sizes=${SIZES:-32 64 128 256 512 1024}
repeats=${REPEATS:-3}
threads=${SLURM_CPUS_PER_TASK:-8}
cutoff=${CUTOFF:-64}
export OMP_PROC_BIND=${OMP_PROC_BIND:-close}
export OMP_PLACES=${OMP_PLACES:-cores}

# Admission check with the known product: every output entry is 512.
./matmul 512 1 1 "$cutoff" ones >/dev/null

printf '%s\n' 'n,method,threads,seconds'
for n in $sizes; do
    ./matmul "$n" "$repeats" "$threads" "$cutoff"
done
