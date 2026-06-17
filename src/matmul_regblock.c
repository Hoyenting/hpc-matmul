#include "matmul.h"

/*
 * Register micro-kernel size: 4 rows x 4 columns.
 * These 16 accumulators live in registers throughout the K loop,
 * avoiding repeated load/store to C until the very end of each tile.
 */
#define RM 4
#define RN 4

#ifndef MATMUL_REGBLOCK_BM
#define MATMUL_REGBLOCK_BM 128
#endif
#ifndef MATMUL_REGBLOCK_BK
#define MATMUL_REGBLOCK_BK 8
#endif
#ifndef MATMUL_REGBLOCK_BN
#define MATMUL_REGBLOCK_BN 128
#endif

static size_t g_bm = MATMUL_REGBLOCK_BM;
static size_t g_bk = MATMUL_REGBLOCK_BK;
static size_t g_bn = MATMUL_REGBLOCK_BN;

void matmul_regblock_set_tiles(size_t BM, size_t BK, size_t BN) {
    if (BM > 0) g_bm = BM;
    if (BK > 0) g_bk = BK;
    if (BN > 0) g_bn = BN;
}

void matmul_regblock_get_tiles(size_t *BM, size_t *BK, size_t *BN) {
    if (BM) *BM = g_bm;
    if (BK) *BK = g_bk;
    if (BN) *BN = g_bn;
}

size_t matmul_regblock_rm(void) { return RM; }
size_t matmul_regblock_rn(void) { return RN; }

/*
 * Register-blocked matrix multiplication (row-major):
 *   C[M x N] += A[M x K] * B[K x N]
 *
 * Two-level structure:
 *   Outer (cache tiles):    ii-jj-kk, step BM/BN/BK  — fit working set in L1/L2
 *   Inner (register tile):  RM x RN = 4 x 4 accumulators held in registers
 *
 * Key difference from matmul_block: partial sums for a 4x4 output tile are kept
 * in 16 scalar locals (c00..c33) and written back to C only once per kk-tile,
 * eliminating the repeated load-add-store that limited matmul_block.
 */
void matmul_regblock(size_t M, size_t N, size_t K,
                     const double *restrict A, const double *restrict B,
                     double *restrict C) {
    const size_t BM = g_bm;
    const size_t BK = g_bk;
    const size_t BN = g_bn;

    for (size_t ii = 0; ii < M; ii += BM) {
        const size_t i_end = (ii + BM < M) ? (ii + BM) : M;
        for (size_t jj = 0; jj < N; jj += BN) {
            const size_t j_end = (jj + BN < N) ? (jj + BN) : N;
            for (size_t kk = 0; kk < K; kk += BK) {
                const size_t k_end = (kk + BK < K) ? (kk + BK) : K;

                /* --- main 4x4 micro-kernel --- */
                size_t i = ii;
                for (; i + RM <= i_end; i += RM) {
                    size_t j = jj;
                    for (; j + RN <= j_end; j += RN) {
                        double c00 = 0.0, c01 = 0.0, c02 = 0.0, c03 = 0.0;
                        double c10 = 0.0, c11 = 0.0, c12 = 0.0, c13 = 0.0;
                        double c20 = 0.0, c21 = 0.0, c22 = 0.0, c23 = 0.0;
                        double c30 = 0.0, c31 = 0.0, c32 = 0.0, c33 = 0.0;

                        for (size_t k = kk; k < k_end; ++k) {
                            const double a0 = A[(i+0)*K + k];
                            const double a1 = A[(i+1)*K + k];
                            const double a2 = A[(i+2)*K + k];
                            const double a3 = A[(i+3)*K + k];
                            const double b0 = B[k*N + j+0];
                            const double b1 = B[k*N + j+1];
                            const double b2 = B[k*N + j+2];
                            const double b3 = B[k*N + j+3];

                            c00 += a0*b0; c01 += a0*b1; c02 += a0*b2; c03 += a0*b3;
                            c10 += a1*b0; c11 += a1*b1; c12 += a1*b2; c13 += a1*b3;
                            c20 += a2*b0; c21 += a2*b1; c22 += a2*b2; c23 += a2*b3;
                            c30 += a3*b0; c31 += a3*b1; c32 += a3*b2; c33 += a3*b3;
                        }

                        C[(i+0)*N+j+0] += c00; C[(i+0)*N+j+1] += c01;
                        C[(i+0)*N+j+2] += c02; C[(i+0)*N+j+3] += c03;
                        C[(i+1)*N+j+0] += c10; C[(i+1)*N+j+1] += c11;
                        C[(i+1)*N+j+2] += c12; C[(i+1)*N+j+3] += c13;
                        C[(i+2)*N+j+0] += c20; C[(i+2)*N+j+1] += c21;
                        C[(i+2)*N+j+2] += c22; C[(i+2)*N+j+3] += c23;
                        C[(i+3)*N+j+0] += c30; C[(i+3)*N+j+1] += c31;
                        C[(i+3)*N+j+2] += c32; C[(i+3)*N+j+3] += c33;
                    }
                    /* j-edge: columns that don't fill a full RN strip */
                    for (; j < j_end; ++j) {
                        double s0 = 0.0, s1 = 0.0, s2 = 0.0, s3 = 0.0;
                        for (size_t k = kk; k < k_end; ++k) {
                            const double b = B[k*N + j];
                            s0 += A[(i+0)*K + k] * b;
                            s1 += A[(i+1)*K + k] * b;
                            s2 += A[(i+2)*K + k] * b;
                            s3 += A[(i+3)*K + k] * b;
                        }
                        C[(i+0)*N+j] += s0;
                        C[(i+1)*N+j] += s1;
                        C[(i+2)*N+j] += s2;
                        C[(i+3)*N+j] += s3;
                    }
                }
                /* i-edge: rows that don't fill a full RM strip */
                for (; i < i_end; ++i) {
                    for (size_t j = jj; j < j_end; ++j) {
                        double s = 0.0;
                        for (size_t k = kk; k < k_end; ++k) {
                            s += A[i*K + k] * B[k*N + j];
                        }
                        C[i*N + j] += s;
                    }
                }
            }
        }
    }
}
