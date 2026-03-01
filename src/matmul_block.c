#include "matmul.h"

#ifndef MATMUL_BLOCK_BM
#define MATMUL_BLOCK_BM 64
#endif

#ifndef MATMUL_BLOCK_BK
#define MATMUL_BLOCK_BK 32
#endif

#ifndef MATMUL_BLOCK_BN
#define MATMUL_BLOCK_BN 64
#endif

static size_t g_block_bm = MATMUL_BLOCK_BM;
static size_t g_block_bk = MATMUL_BLOCK_BK;
static size_t g_block_bn = MATMUL_BLOCK_BN;

void matmul_block_set_tiles(size_t BM, size_t BK, size_t BN) {
    if (BM > 0) {
        g_block_bm = BM;
    }
    if (BK > 0) {
        g_block_bk = BK;
    }
    if (BN > 0) {
        g_block_bn = BN;
    }
}

void matmul_block_get_tiles(size_t *BM, size_t *BK, size_t *BN) {
    if (BM != NULL) {
        *BM = g_block_bm;
    }
    if (BK != NULL) {
        *BK = g_block_bk;
    }
    if (BN != NULL) {
        *BN = g_block_bn;
    }
}

/*
 * Blocked matrix multiplication (row-major):
 *   C[M x N] += A[M x K] * B[K x N]
 *
 * Outer loops (tiling): ii-jj-kk
 * Inner loops: i-k-j
 */
void matmul_block(size_t M, size_t N, size_t K,
                  const double *A, const double *B, double *restrict C) {
    const size_t BM = g_block_bm;
    const size_t BK = g_block_bk;
    const size_t BN = g_block_bn;
    for (size_t ii = 0; ii < M; ii += BM) {
        const size_t i_end = (ii + BM < M) ? (ii + BM) : M;

        for (size_t jj = 0; jj < N; jj += BN) {
            const size_t j_end = (jj + BN < N) ? (jj + BN) : N;

            for (size_t kk = 0; kk < K; kk += BK) {
                const size_t k_end = (kk + BK < K) ? (kk + BK) : K;

                for (size_t i = ii; i < i_end; ++i) {
                    double *c_row = &C[i * N];
                    for (size_t k = kk; k < k_end; ++k) {
                        const double aik = A[i * K + k];
                        const double *b_row = &B[k * N];
                        for (size_t j = jj; j < j_end; ++j) {
                            c_row[j] += aik * b_row[j];
                        }
                    }
                }
            }
        }
    }
}
