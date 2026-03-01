#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <mach/mach_time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "matmul.h"

typedef void (*matmul_kernel_t)(size_t M, size_t N, size_t K,
                                const double *A, const double *B,
                                double *restrict C);

typedef enum kernel_kind {
    KERNEL_NAIVE = 0,
    KERNEL_ROWMAJOR = 1,
    KERNEL_BLOCK = 2
} kernel_kind_t;

static volatile double g_sink = 0.0;

static void usage(const char *prog) {
    fprintf(stderr,
            "Usage:\n"
            "  %s [--kernel naive|rowmajor|block] [--verify] [--tol TOL]\n"
            "  %s [--kernel naive|rowmajor|block] [--verify] [--tol TOL] [--bm BM] [--bk BK] [--bn BN] N [repeats]\n"
            "  %s [--kernel naive|rowmajor|block] [--verify] [--tol TOL] [--bm BM] [--bk BK] [--bn BN] M N K [repeats]\n",
            prog, prog, prog);
}

static int resolve_kernel(const char *name, matmul_kernel_t *fn_out,
                          kernel_kind_t *kind_out) {
    if (strcmp(name, "naive") == 0) {
        *fn_out = matmul_naive;
        *kind_out = KERNEL_NAIVE;
        return 0;
    }
    if (strcmp(name, "rowmajor") == 0) {
        *fn_out = matmul_naive_rowmajor;
        *kind_out = KERNEL_ROWMAJOR;
        return 0;
    }
    if (strcmp(name, "block") == 0 || strcmp(name, "blocked") == 0) {
        *fn_out = matmul_block;
        *kind_out = KERNEL_BLOCK;
        return 0;
    }
    return -1;
}

static int parse_size_arg(const char *s, size_t *out) {
    char *end = NULL;
    errno = 0;
    unsigned long long v = strtoull(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || v == 0ULL) {
        return -1;
    }
    *out = (size_t)v;
    return 0;
}

static int parse_double_arg(const char *s, double *out) {
    char *end = NULL;
    errno = 0;
    double v = strtod(s, &end);
    if (errno != 0 || end == s || *end != '\0' || !isfinite(v)) {
        return -1;
    }
    *out = v;
    return 0;
}

static int mul_overflow_size(size_t a, size_t b, size_t *out) {
    if (a != 0 && b > SIZE_MAX / a) {
        return 1;
    }
    *out = a * b;
    return 0;
}

static double now_sec(void) {
    static mach_timebase_info_data_t timebase = {0, 0};
    if (timebase.denom == 0) {
        (void)mach_timebase_info(&timebase);
    }
    uint64_t t = mach_absolute_time();
    long double ns = (long double)t * (long double)timebase.numer /
                     (long double)timebase.denom;
    return (double)(ns * 1e-9L);
}

static void *aligned_alloc_or_null(size_t alignment, size_t bytes) {
    void *ptr = NULL;
    if (posix_memalign(&ptr, alignment, bytes) != 0) {
        return NULL;
    }
    return ptr;
}

static void fill_random(double *x, size_t n, uint64_t seed) {
    uint64_t state = seed ? seed : 0x9e3779b97f4a7c15ULL;
    for (size_t i = 0; i < n; ++i) {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        uint64_t z = state * 2685821657736338717ULL;
        x[i] = ((double)(z >> 11) * (1.0 / 9007199254740992.0)) - 0.5;
    }
}

static int cmp_double_asc(const void *a, const void *b) {
    const double da = *(const double *)a;
    const double db = *(const double *)b;
    return (da > db) - (da < db);
}

typedef struct stats {
    double min;
    double median;
    double mean;
    double stddev;
} stats_t;

static int compute_stats(const double *values, size_t n, stats_t *out) {
    double *sorted = (double *)malloc(n * sizeof(double));
    if (!sorted) {
        return -1;
    }

    double sum = 0.0;
    double min_v = values[0];
    for (size_t i = 0; i < n; ++i) {
        sorted[i] = values[i];
        sum += values[i];
        if (values[i] < min_v) {
            min_v = values[i];
        }
    }
    const double mean = sum / (double)n;

    double var_acc = 0.0;
    for (size_t i = 0; i < n; ++i) {
        const double d = values[i] - mean;
        var_acc += d * d;
    }
    const double stddev = sqrt(var_acc / (double)n);

    qsort(sorted, n, sizeof(double), cmp_double_asc);
    double median = 0.0;
    if ((n & 1U) != 0U) {
        median = sorted[n / 2];
    } else {
        median = 0.5 * (sorted[n / 2 - 1] + sorted[n / 2]);
    }

    out->min = min_v;
    out->mean = mean;
    out->median = median;
    out->stddev = stddev;
    free(sorted);
    return 0;
}

static int verify_result(const double *C_ref, const double *C_test, size_t n,
                         double tol, double *max_abs_err_out,
                         double *max_rel_err_out) {
    double max_abs_err = 0.0;
    double max_rel_err = 0.0;
    int ok = 1;

    for (size_t i = 0; i < n; ++i) {
        const double ref = C_ref[i];
        const double test = C_test[i];
        const double abs_err = fabs(test - ref);
        const double denom = fabs(ref) > 1e-300 ? fabs(ref) : 1.0;
        const double rel_err = abs_err / denom;

        if (abs_err > max_abs_err) {
            max_abs_err = abs_err;
        }
        if (rel_err > max_rel_err) {
            max_rel_err = rel_err;
        }
        if (!(abs_err <= tol || rel_err <= tol)) {
            ok = 0;
        }
    }

    *max_abs_err_out = max_abs_err;
    *max_rel_err_out = max_rel_err;
    return ok;
}

int main(int argc, char **argv) {
    size_t M = 1024, N = 1024, K = 1024;
    size_t repeats = 5;
    matmul_kernel_t kernel = matmul_naive;
    kernel_kind_t kernel_kind = KERNEL_NAIVE;
    size_t opt_bm = 0, opt_bk = 0, opt_bn = 0;
    int verify = 0;
    double tol = 1e-9;
    int argi = 1;

    while (argi < argc) {
        if (strcmp(argv[argi], "--verify") == 0) {
            verify = 1;
            ++argi;
            continue;
        }
        if (strcmp(argv[argi], "--kernel") == 0 || strcmp(argv[argi], "-k") == 0) {
            if (argi + 1 >= argc) {
                usage(argv[0]);
                return 1;
            }
            if (resolve_kernel(argv[argi + 1], &kernel, &kernel_kind) != 0) {
                fprintf(stderr, "unknown kernel: %s\n", argv[argi + 1]);
                usage(argv[0]);
                return 1;
            }
            argi += 2;
            continue;
        }
        if (strcmp(argv[argi], "--bm") == 0) {
            if (argi + 1 >= argc || parse_size_arg(argv[argi + 1], &opt_bm) != 0) {
                fprintf(stderr, "invalid --bm value\n");
                usage(argv[0]);
                return 1;
            }
            argi += 2;
            continue;
        }
        if (strcmp(argv[argi], "--bk") == 0) {
            if (argi + 1 >= argc || parse_size_arg(argv[argi + 1], &opt_bk) != 0) {
                fprintf(stderr, "invalid --bk value\n");
                usage(argv[0]);
                return 1;
            }
            argi += 2;
            continue;
        }
        if (strcmp(argv[argi], "--bn") == 0) {
            if (argi + 1 >= argc || parse_size_arg(argv[argi + 1], &opt_bn) != 0) {
                fprintf(stderr, "invalid --bn value\n");
                usage(argv[0]);
                return 1;
            }
            argi += 2;
            continue;
        }
        if (strcmp(argv[argi], "--tol") == 0) {
            if (argi + 1 >= argc || parse_double_arg(argv[argi + 1], &tol) != 0 ||
                tol <= 0.0) {
                fprintf(stderr, "invalid --tol value\n");
                usage(argv[0]);
                return 1;
            }
            argi += 2;
            continue;
        }
        break;
    }

    const int rem = argc - argi;
    if (rem == 1 || rem == 2) {
        if (parse_size_arg(argv[argi], &N) != 0) {
            usage(argv[0]);
            return 1;
        }
        M = N;
        K = N;
        if (rem == 2 && parse_size_arg(argv[argi + 1], &repeats) != 0) {
            usage(argv[0]);
            return 1;
        }
    } else if (rem == 3 || rem == 4) {
        if (parse_size_arg(argv[argi], &M) != 0 ||
            parse_size_arg(argv[argi + 1], &N) != 0 ||
            parse_size_arg(argv[argi + 2], &K) != 0) {
            usage(argv[0]);
            return 1;
        }
        if (rem == 4 && parse_size_arg(argv[argi + 3], &repeats) != 0) {
            usage(argv[0]);
            return 1;
        }
    } else if (rem != 0) {
        usage(argv[0]);
        return 1;
    }

    if (opt_bm > 0 || opt_bk > 0 || opt_bn > 0) {
        size_t bm = 0, bk = 0, bn = 0;
        matmul_block_get_tiles(&bm, &bk, &bn);
        if (opt_bm > 0) {
            bm = opt_bm;
        }
        if (opt_bk > 0) {
            bk = opt_bk;
        }
        if (opt_bn > 0) {
            bn = opt_bn;
        }
        matmul_block_set_tiles(bm, bk, bn);
    }

    if (repeats == 0) {
        fprintf(stderr, "repeats must be >= 1\n");
        return 1;
    }
    if (M < 1024 || N < 1024 || K < 1024) {
        fprintf(stderr,
                "Warning: dimensions under 1024 are usually too small for stable profiling.\n");
    }

    size_t elems_a = 0, elems_b = 0, elems_c = 0;
    size_t bytes_a = 0, bytes_b = 0, bytes_c = 0;
    if (mul_overflow_size(M, K, &elems_a) || mul_overflow_size(K, N, &elems_b) ||
        mul_overflow_size(M, N, &elems_c) ||
        mul_overflow_size(elems_a, sizeof(double), &bytes_a) ||
        mul_overflow_size(elems_b, sizeof(double), &bytes_b) ||
        mul_overflow_size(elems_c, sizeof(double), &bytes_c)) {
        fprintf(stderr, "matrix size overflow\n");
        return 1;
    }

    const size_t alignment = 128;
    double *A = (double *)aligned_alloc_or_null(alignment, bytes_a);
    double *B = (double *)aligned_alloc_or_null(alignment, bytes_b);
    double *C = (double *)aligned_alloc_or_null(alignment, bytes_c);
    double *clear_times = (double *)malloc(repeats * sizeof(double));
    double *compute_times = (double *)malloc(repeats * sizeof(double));
    double *total_times = (double *)malloc(repeats * sizeof(double));
    if (!A || !B || !C || !clear_times || !compute_times || !total_times) {
        fprintf(stderr,
                "allocation failed (A=%p, B=%p, C=%p, clear=%p, compute=%p, total=%p)\n",
                (void *)A, (void *)B, (void *)C, (void *)clear_times,
                (void *)compute_times, (void *)total_times);
        free(A);
        free(B);
        free(C);
        free(clear_times);
        free(compute_times);
        free(total_times);
        return 1;
    }

    fill_random(A, elems_a, 0x123456789abcdef0ULL);
    fill_random(B, elems_b, 0x0fedcba987654321ULL);

    if (verify) {
        double *C_ref = (double *)aligned_alloc_or_null(alignment, bytes_c);
        if (!C_ref) {
            fprintf(stderr, "verification allocation failed (C_ref)\n");
            free(A);
            free(B);
            free(C);
            free(clear_times);
            free(compute_times);
            free(total_times);
            return 1;
        }

        memset(C_ref, 0, bytes_c);
        matmul_naive(M, N, K, A, B, C_ref);
        memset(C, 0, bytes_c);
        kernel(M, N, K, A, B, C);

        double max_abs_err = 0.0;
        double max_rel_err = 0.0;
        int ok = verify_result(C_ref, C, elems_c, tol, &max_abs_err, &max_rel_err);

        printf("verify=%s tol=%.3e max_abs_err=%.3e max_rel_err=%.3e\n",
               ok ? "PASS" : "FAIL", tol, max_abs_err, max_rel_err);

        free(C_ref);
        if (!ok) {
            fprintf(stderr,
                    "verification failed: error exceeds tolerance (tol=%.3e)\n", tol);
            free(A);
            free(B);
            free(C);
            free(clear_times);
            free(compute_times);
            free(total_times);
            return 2;
        }
    }

    memset(C, 0, bytes_c);
    kernel(M, N, K, A, B, C);

    for (size_t r = 0; r < repeats; ++r) {
        const double t_total0 = now_sec();
        const double t_clear0 = now_sec();
        memset(C, 0, bytes_c);
        const double t_clear1 = now_sec();
        const double t_compute0 = t_clear1;
        kernel(M, N, K, A, B, C);
        const double t_compute1 = now_sec();
        clear_times[r] = t_clear1 - t_clear0;
        compute_times[r] = t_compute1 - t_compute0;
        total_times[r] = t_compute1 - t_total0;
        g_sink += C[r % elems_c];
    }

    stats_t clear_stats;
    stats_t compute_stats_v;
    stats_t total_stats;
    if (compute_stats(clear_times, repeats, &clear_stats) != 0 ||
        compute_stats(compute_times, repeats, &compute_stats_v) != 0 ||
        compute_stats(total_times, repeats, &total_stats) != 0) {
        fprintf(stderr, "failed to compute timing statistics\n");
        free(A);
        free(B);
        free(C);
        free(clear_times);
        free(compute_times);
        free(total_times);
        return 1;
    }

    const double flops = 2.0 * (double)M * (double)N * (double)K;
    const double gflops_compute_best = flops / compute_stats_v.min / 1e9;
    const double gflops_compute_median = flops / compute_stats_v.median / 1e9;
    const double gflops_compute_mean = flops / compute_stats_v.mean / 1e9;
    const double gflops_total_best = flops / total_stats.min / 1e9;
    const double gflops_total_median = flops / total_stats.median / 1e9;
    const double gflops_total_mean = flops / total_stats.mean / 1e9;
    const double gib_total =
        (double)(bytes_a + bytes_b + bytes_c) / (1024.0 * 1024.0 * 1024.0);
    char kernel_label[128];
    if (kernel_kind == KERNEL_NAIVE) {
        (void)snprintf(kernel_label, sizeof(kernel_label), "naive(i-j-k)");
    } else if (kernel_kind == KERNEL_ROWMAJOR) {
        (void)snprintf(kernel_label, sizeof(kernel_label), "rowmajor(i-k-j)");
    } else {
        size_t bm = 0, bk = 0, bn = 0;
        matmul_block_get_tiles(&bm, &bk, &bn);
        (void)snprintf(kernel_label, sizeof(kernel_label),
                       "blocked(ii-jj-kk, i-k-j) BM=%zu BN=%zu BK=%zu", bm, bn, bk);
    }

    printf("------------------------------------------------------------\n");
    printf("matmul baseline (double, row-major)\n");
    printf("kernel=%s\n", kernel_label);
    printf("M=%zu N=%zu K=%zu repeats=%zu alignment=%zuB\n", M, N, K, repeats,
           alignment);
    printf("A=%.3f MiB B=%.3f MiB C=%.3f MiB total=%.3f GiB\n",
           (double)bytes_a / (1024.0 * 1024.0), (double)bytes_b / (1024.0 * 1024.0),
           (double)bytes_c / (1024.0 * 1024.0), gib_total);
    printf("clear_time:   min=%.6f s  median=%.6f s  mean=%.6f s  stddev=%.6f s\n",
           clear_stats.min, clear_stats.median, clear_stats.mean,
           clear_stats.stddev);
    printf("compute_time: min=%.6f s  median=%.6f s  mean=%.6f s  stddev=%.6f s\n",
           compute_stats_v.min, compute_stats_v.median, compute_stats_v.mean,
           compute_stats_v.stddev);
    printf("total_time:   min=%.6f s  median=%.6f s  mean=%.6f s  stddev=%.6f s\n",
           total_stats.min, total_stats.median, total_stats.mean,
           total_stats.stddev);
    printf("gflops_compute: best=%.2f  median=%.2f  mean=%.2f\n",
           gflops_compute_best, gflops_compute_median, gflops_compute_mean);
    printf("gflops_total:   best=%.2f  median=%.2f  mean=%.2f\n", gflops_total_best,
           gflops_total_median, gflops_total_mean);
    printf("checksum_guard=%.6e\n", (double)g_sink);
    printf("------------------------------------------------------------\n");

    free(A);
    free(B);
    free(C);
    free(clear_times);
    free(compute_times);
    free(total_times);
    return 0;
}
