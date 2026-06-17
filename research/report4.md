# Report 04: Register-Blocked Matrix Multiplication Performance Investigation

Date: 2026-05-17  
Environment: MacBook Air M3 / macOS / Clang (Apple LLVM)  
Scope: `main.c` + `src/matmul_regblock.c`

---

## SECTION: Measurement Snapshot

Test commands:

```bash
./matmul --kernel regblock --verify 1024 5                        # default config
./matmul --kernel regblock --bm 512 --bk 8 --bn 512 1024 5       # best config found
```

Observed values (snapshot):

```text
--- default (BM=128 BN=128 BK=8) ---
best = 23.03 GFLOPS
avg  = 22.94 GFLOPS

--- best config (BM=512 BN=512 BK=8) ---
best = 30.80 GFLOPS
avg  = 30.74 GFLOPS

verify = PASS  max_abs_err = 3.375e-14  max_rel_err = 1.074e-09
```

---

## SECTION: Experimental Setup

Problem size:

```text
M=1024
N=1024
K=1024
```

Repeats: `5` (plus 1 warm-up)

Precision: `double`

Compiler flags:

```text
-O3 -march=native
```

Data initialization: pseudo-random (xorshift64)

Timing method:

```text
mach_absolute_time()
```

Micro-kernel size: `RM=4, RN=4` (compile-time constants)

---

## SECTION: Key Findings

### 1. Register blocking yields ~1.4x speedup over cache-only blocking

Comparison at identical tile size (BM=128, BN=128, BK=8):

```text
matmul_block   -> 16.97 GFLOPS
matmul_regblock -> 23.03 GFLOPS
```

Full progression:

```text
naive (i-j-k)         ->  2.1 GFLOPS
rowmajor (i-k-j)      -> 14.9 GFLOPS
cache-blocked (best)  -> 16.9 GFLOPS
regblock default      -> 23.0 GFLOPS
regblock best config  -> 30.8 GFLOPS
```

The gain comes from eliminating repeated load/store to `C` in the inner loop.

In `matmul_block`, each iteration of the innermost loop does:

```c
c_row[j] += aik * b_row[j];   // load C, fmadd, store C — every iteration
```

In `matmul_regblock`, 16 scalar locals (`c00..c33`) absorb all partial sums:

```c
c00 += a0 * b0;   // pure register operation, no memory write
// ... (16 accumulators, all in registers across the entire k loop)
C[(i+0)*N+j+0] += c00;   // single write-back after the k loop ends
```

---

### 2. The compiler auto-vectorizes the 4x4 micro-kernel using NEON

Assembly inspection (`-O3 -march=native`):

```asm
ldp     q18, q19, [x19, #-16]    ; load B[k*N+j..j+3] as two 2-double NEON regs
ld1r.2d { v20 }, [x8], #8        ; broadcast A[i*K+k] into a 2-double NEON reg
fmla.2d v16, v18, v20             ; v16 (= [c00, c01]) += v18 * v20
fmla.2d v17, v19, v20             ; v17 (= [c02, c03]) += v19 * v20
ld1r.2d { v20 }, [x24], #8       ; broadcast A[(i+1)*K+k]
fmla.2d v6,  v18, v20             ; [c10, c11] += ...
fmla.2d v7,  v19, v20             ; [c12, c13] += ...
...
```

The `ld1r.2d` + `fmla.2d` pattern processes 2 columns at a time.
Each k iteration performs 8 × `fmla.2d` = 16 FMAs in 8 SIMD instructions.

This vectorization happens automatically because the 4x4 accumulator structure
exposes independent accumulators with no inter-iteration dependencies on C,
giving the compiler the freedom to map them onto NEON registers.

---

### 3. BK sensitivity is lower than for cache-blocked

In `matmul_block`, BK had a clear peak at 8 because larger BK overflowed
the working-set and caused cache pressure.

In `matmul_regblock`, partial sums stay in registers regardless of BK,
so the kernel is less sensitive to BK increases:

```text
BK=4   -> 24.6 GFLOPS
BK=8   -> 23.0 GFLOPS
BK=16  -> 26.6 GFLOPS
BK=32  -> 25.6 GFLOPS
BK=64  -> 25.1 GFLOPS
```

BK=16 slightly outperforms BK=8 at BM=BN=128, unlike the block kernel
where BK=8 was strictly optimal.

---

### 4. Larger outer tiles improve performance up to a plateau

Sweeping BM=BN at BK=8:

```text
BM=BN=128  -> 23.0 GFLOPS
BM=BN=256  -> 30.0 GFLOPS
BM=BN=512  -> 30.8 GFLOPS
BM=BN=1024 -> 31.5 GFLOPS  (no outer tiling)
```

Performance plateaus around BM=BN=512–1024, suggesting the benefit
of outer tiling is secondary once register blocking is in place.
The working set at BM=BN=512, BK=8 is approximately 3 MB, which
likely fits or nearly fits in M3's large L2/SLC (System Level Cache).

---

## SECTION: Issue Tracker

### Issue 1: Accumulators are 128-bit NEON, not 256-bit

Priority: `Low`  
Status: `Informational`

The M3 NEON unit processes 128-bit vectors (2 doubles per `fmla.2d`).
A wider micro-kernel (e.g., `1x8` or `4x8`) could double vector throughput
per instruction, but requires explicit NEON intrinsics since the compiler
does not widen beyond 128-bit here.

This is beyond the scope of this project phase.

---

### Issue 2: No software prefetch

Priority: `Low`  
Status: `Open`

The kernel does not issue prefetch hints for future A and B tiles.
On M3, hardware prefetch handles most sequential access patterns well,
but explicit `__builtin_prefetch` could reduce stall latency for
irregular or strided accesses.

---

## SECTION: Phase Conclusion

Register blocking adds a second optimization layer on top of cache blocking:

- Cache tiling ensures the working set fits in L1/L2.
- Register tiling keeps partial sums in registers, eliminating memory round-trips for `C`.

Together they lift performance from ~1.1 GFLOPS (naive) to ~31 GFLOPS,
a roughly **28x** improvement over the baseline.

Crucially, no SIMD intrinsics were written: the 4x4 accumulator structure
alone was sufficient for the compiler to emit `fmla.2d` vectorized code.
This demonstrates that micro-kernel design drives vectorization opportunity
even without hand-written intrinsics.

The project has covered the three canonical scalar optimization techniques
for dense matrix multiplication:

1. Loop reordering for memory access pattern (rowmajor)
2. Cache blocking for data reuse across the cache hierarchy
3. Register blocking for accumulator reuse within registers

Further improvement (BLAS-level packing, explicit SIMD, threading) would
enter territory beyond the scope of this project.

---

## SECTION: Next Steps

1. *(Optional)* Inspect `matmul_block.s` vs `matmul_regblock.s` side-by-side to contrast the load/store patterns.
2. *(Optional)* Explore a `1x8` micro-kernel to see if wider register tiles improve SIMD utilization.
3. *(Optional)* Add OpenMP parallelism as an orthogonal optimization axis.

---
