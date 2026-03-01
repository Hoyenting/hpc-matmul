#include "matmul.h"

/*
 * Baseline matrix multiplication (row-major):
 *   C[M x N] += A[M x K] * B[K x N]
 */
void matmul_naive(size_t M, size_t N, size_t K,
                  const double *A, const double *B, double *restrict C) {
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            double sum = 0.0;
            for (size_t k = 0; k < K; ++k) {
                sum += A[i * K + k] * B[k * N + j];
            }
            C[i * N + j] += sum;
        }
    }
}
