CC = gcc
MPICC = mpicc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Wpedantic

.PHONY: all
all: loops loops_mpi

loops: loops.c loops.h
	$(CC) $(CFLAGS) -fopenmp $< -o $@ -lm

loops_mpi: loops_mpi.c loops.h
	$(MPICC) $(CFLAGS) $< -o $@ -lm
