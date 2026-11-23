CC = gcc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Wpedantic

matmul: matmul.c
	$(CC) $(CFLAGS) -fopenmp $< -o $@ -lm
