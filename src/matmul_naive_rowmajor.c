#include "matmul.h"

/*
 * Row-major friendly baseline:
 *   C[M x N] += A[M x K] * B[K x N]
 * Loop order: i-k-j
 */
void matmul_naive_rowmajor(size_t M, size_t N, size_t K,
                           const double *A, const double *B,
                           double *restrict C) {
    for (size_t i = 0; i < M; ++i) {
        double *c_row = &C[i * N];
        for (size_t k = 0; k < K; ++k) {
            const double aik = A[i * K + k];
            const double *b_row = &B[k * N];
            for (size_t j = 0; j < N; ++j) {
                c_row[j] += aik * b_row[j];
            }
        }
    }
}
