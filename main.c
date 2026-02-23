#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <mach/mach_time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void matmul_naive(size_t M, size_t N, size_t K,
                  const double *A, const double *B, double *restrict C);
void matmul_naive_rowmajor(size_t M, size_t N, size_t K,
                           const double *A, const double *B,
                           double *restrict C);

typedef void (*matmul_kernel_t)(size_t M, size_t N, size_t K,
                                const double *A, const double *B,
                                double *restrict C);

static volatile double g_sink = 0.0;

static void usage(const char *prog) {
    fprintf(stderr,
            "Usage:\n"
            "  %s [--kernel naive|rowmajor] [--verify] [--tol TOL]\n"
            "  %s [--kernel naive|rowmajor] [--verify] [--tol TOL] N [repeats]\n"
            "  %s [--kernel naive|rowmajor] [--verify] [--tol TOL] M N K [repeats]\n",
            prog, prog, prog);
}

static int resolve_kernel(const char *name, matmul_kernel_t *fn_out,
                          const char **label_out) {
    if (strcmp(name, "naive") == 0) {
        *fn_out = matmul_naive;
        *label_out = "naive(i-j-k)";
        return 0;
    }
    if (strcmp(name, "rowmajor") == 0) {
        *fn_out = matmul_naive_rowmajor;
        *label_out = "rowmajor(i-k-j)";
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
    const char *kernel_label = "naive(i-j-k)";
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
            if (resolve_kernel(argv[argi + 1], &kernel, &kernel_label) != 0) {
                fprintf(stderr, "unknown kernel: %s\n", argv[argi + 1]);
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
    double *times = (double *)malloc(repeats * sizeof(double));
    if (!A || !B || !C || !times) {
        fprintf(stderr, "allocation failed (A=%p, B=%p, C=%p, times=%p)\n", (void *)A,
                (void *)B, (void *)C, (void *)times);
        free(A);
        free(B);
        free(C);
        free(times);
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
            free(times);
            return 1;
        }

        matmul_naive(M, N, K, A, B, C_ref);
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
            free(times);
            return 2;
        }
    }

    kernel(M, N, K, A, B, C);

    double min_sec = 1e300;
    double sum_sec = 0.0;
    for (size_t r = 0; r < repeats; ++r) {
        double t0 = now_sec();
        kernel(M, N, K, A, B, C);
        double t1 = now_sec();
        double dt = t1 - t0;
        times[r] = dt;
        if (dt < min_sec) {
            min_sec = dt;
        }
        sum_sec += dt;
        g_sink += C[r % elems_c];
    }

    const double mean_sec = sum_sec / (double)repeats;
    double var_acc = 0.0;
    for (size_t r = 0; r < repeats; ++r) {
        const double d = times[r] - mean_sec;
        var_acc += d * d;
    }
    const double stddev_sec = sqrt(var_acc / (double)repeats);

    qsort(times, repeats, sizeof(double), cmp_double_asc);
    double median_sec = 0.0;
    if ((repeats & 1U) != 0U) {
        median_sec = times[repeats / 2];
    } else {
        median_sec = 0.5 * (times[repeats / 2 - 1] + times[repeats / 2]);
    }

    const double flops = 2.0 * (double)M * (double)N * (double)K;
    const double gflops_best = flops / min_sec / 1e9;
    const double gflops_median = flops / median_sec / 1e9;
    const double gflops_mean = flops / mean_sec / 1e9;
    const double gib_total =
        (double)(bytes_a + bytes_b + bytes_c) / (1024.0 * 1024.0 * 1024.0);

    printf("matmul baseline (double, row-major)\n");
    printf("kernel=%s\n", kernel_label);
    printf("M=%zu N=%zu K=%zu repeats=%zu alignment=%zuB\n", M, N, K, repeats,
           alignment);
    printf("A=%.3f MiB B=%.3f MiB C=%.3f MiB total=%.3f GiB\n",
           (double)bytes_a / (1024.0 * 1024.0), (double)bytes_b / (1024.0 * 1024.0),
           (double)bytes_c / (1024.0 * 1024.0), gib_total);
    printf("min_time=%.6f s  median_time=%.6f s\n", min_sec, median_sec);
    printf("mean_time=%.6f s  stddev_time=%.6f s\n", mean_sec, stddev_sec);
    printf("gflops_best=%.2f  gflops_median=%.2f  gflops_mean=%.2f\n", gflops_best,
           gflops_median, gflops_mean);
    printf("checksum_guard=%.6e\n", (double)g_sink);

    free(A);
    free(B);
    free(C);
    free(times);
    return 0;
}
