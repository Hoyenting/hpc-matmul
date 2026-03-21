# Report 03: Cache-Blocked Matrix Multiplication Performance Investigation

Date: 2026-03-11  
Environment: MacBook Air M3 / macOS / Clang (Apple LLVM)  
Scope: `main.c` + `src/matmul_block.c`

---

## SECTION: Measurement Snapshot

Test commands:

```bash
./matmul --kernel block --bm 128 --bn 128 --bk 8 1024 5
./matmul --kernel block --bm 128 --bn 128 --bk 4 1024 5
./matmul --kernel block --bm 128 --bn 128 --bk 16 1024 5
./matmul --kernel block --bm 128 --bn 128 --bk 32 1024 5
./matmul --kernel block --bm 128 --bn 128 --bk 64 1024 5
```

Observed values (snapshot):

```text
best_time = 0.214 s
avg_time  = 0.216 s

best = 10.02 GFLOPS
avg  = 9.96 GFLOPS

checksum_guard = -7.099788e-01
```

---

## SECTION: Experimental Setup

Problem size:

```text
M=1024
N=1024
K=1024
```

Repeats: `5`

Precision: `double`

Compiler flags:

```text
-O3 -march=native
```

Data initialization: pseudo-random initialization

Timing method:

```text
mach_absolute_time()
```

Warm-up iterations: `1`

---

## SECTION: Key Findings

### 1. Increasing tile size significantly improves performance

Observation:

When tile size increases from `16x16` to `128x128`, performance improves from about `3.4 GFLOPS` to around `10 GFLOPS`.

Impact:

This indicates that cache blocking successfully improves data locality. Larger tiles allow better reuse of matrix elements within the cache hierarchy.

Evidence:

```text
16x16x8   -> 3.39 GFLOPS
32x32x8   -> 4.31 GFLOPS
64x64x8   -> 7.60 GFLOPS
128x128x8 -> 10.01 GFLOPS
```

This trend strongly suggests improved temporal locality.

---

### 2. Performance peaks around BK = 8

Observation:

When `BM=128` and `BN=128`, the best performance occurs at `BK=8`.

Increasing `BK` beyond this point reduces performance.

Measured results:

```text
BK=4   -> 9.45 GFLOPS
BK=8   -> 10.01 GFLOPS
BK=16  -> 7.84 GFLOPS
BK=32  -> 7.25 GFLOPS
BK=64  -> 7.09 GFLOPS
```

Impact:

Deeper K blocking does not improve compute efficiency in the current kernel.

Instead, larger `BK` increases:

- working set size
- register pressure
- load/store traffic

without providing sufficient additional reuse.

---

### 3. The implementation improves locality but not compute efficiency

Observation:

Even with blocking, the kernel still performs accumulation directly into memory-backed elements of matrix `C`.

Inner loop structure:

```c
c_row[j] += aik * b_row[j]
```

Each iteration performs:

- load `C`
- multiply-add
- store `C`

Impact:

Partial sums are repeatedly written back to memory instead of remaining in registers.

This means the kernel benefits from cache-level optimization but does not exploit register-level reuse.

Evidence:

Direct inspection of the inner loop structure.

---

## SECTION: Issue Tracker

### Issue 1: Lack of register blocking

Priority: `High`  
Status: `Open`

Possible cause:

The current kernel accumulates directly into memory-backed `C` elements instead of keeping partial sums in registers.

Proposed solution:

Introduce register blocking such as:

- `1x4` accumulator
- `1x8` accumulator
- `4x4` micro-kernel

---

### Issue 2: Limited SIMD utilization

Priority: `Medium`  
Status: `Open`

Possible cause:

Although the inner loop accesses contiguous memory, the compiler may not generate optimal SIMD instructions due to:

- pointer aliasing
- generic loop bounds
- lack of explicit unrolling

Proposed solution:

- Inspect generated assembly
- Enable compiler vectorization reports
- Restructure loops to be more SIMD-friendly

---

### Issue 3: Larger BK increases working-set pressure

Priority: `Medium`  
Status: `Open`

Possible cause:

Larger `BK` increases the working set:

```text
A_block = BM x BK
B_block = BK x BN
C_block = BM x BN
```

This may exceed the optimal cache footprint.

Proposed solution:

Keep `BK` relatively small (around `8`) until register blocking is introduced.

---

## SECTION: Phase Conclusion

Cache blocking significantly improves performance compared to naive implementations.

The current implementation reaches approximately `10 GFLOPS` but appears limited by insufficient register-level reuse.

Further improvements likely require register blocking or micro-kernel design.

The project is ready to move to the next optimization phase.

---

## SECTION: Next Steps

1. Implement register blocking, such as `1x4` or `1x8` accumulator tiles.
2. Inspect compiler-generated assembly to confirm SIMD vectorization.
3. Evaluate performance impact of explicit loop unrolling and SIMD-friendly kernels.

---
