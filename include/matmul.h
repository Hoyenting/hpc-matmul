#ifndef MATMUL_H
#define MATMUL_H

#include <stddef.h>

void matmul_naive(size_t M, size_t N, size_t K,
                  const double *restrict A, const double *restrict B,
                  double *restrict C);

void matmul_naive_rowmajor(size_t M, size_t N, size_t K,
                           const double *restrict A,
                           const double *restrict B,
                           double *restrict C);

void matmul_block(size_t M, size_t N, size_t K,
                  const double *restrict A, const double *restrict B,
                  double *restrict C);

void matmul_block_set_tiles(size_t BM, size_t BK, size_t BN);
void matmul_block_get_tiles(size_t *BM, size_t *BK, size_t *BN);

#endif
