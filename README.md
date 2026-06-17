# hpc-matmul

矩陣乘法最佳化與 profiling 專案，目標是以可重現、可比較的方式，逐階段從 baseline 推進到進階優化版本。

## README Scope

本文件聚焦「成果展示」與「實驗方法」。  
階段性問題與研究觀察請見 `research/` 目錄下各 report。

## Repository Structure

- `main.c`: benchmarking harness（隨機初始化、計時、GFLOPS 輸出、correctness 驗證）
- `src/matmul_naive.c`: Phase 0 — naive i-j-k baseline
- `src/matmul_naive_rowmajor.c`: Phase 1 — loop reorder i-k-j
- `src/matmul_block.c`: Phase 2 — cache-blocked tiling
- `src/matmul_regblock.c`: Phase 3 — register-blocked micro-kernel (4×4)
- `include/matmul.h`: 所有 kernel 的公開介面
- `research/`: 各 phase 的問題紀錄與觀察報告
- `bench/`, `tests/`, `results/`, `scripts/`: 後續擴充用目錄

## Optimization Phases

### Phase 0: Naive Baseline

`src/matmul_naive.c` — i-j-k loop order，B 矩陣跨列存取，cache miss 頻繁。

```
~2.1 GFLOPS  (N=1024, Apple M3, -O3 -march=native)
```

### Phase 1: Loop Reorder

`src/matmul_naive_rowmajor.c` — 改為 i-k-j loop order，B 矩陣由跨列改為循序存取，顯著改善 spatial locality。

```
~14.9 GFLOPS
```

### Phase 2: Cache Blocking

`src/matmul_block.c` — 外層 ii-jj-kk tiling（BM×BK×BN 區塊），讓工作集合留在 L1/L2 快取內，提升 temporal locality。

```
~16.9 GFLOPS  (BM=128, BN=128, BK=8)
```

### Phase 3: Register Blocking

`src/matmul_regblock.c` — 在 cache tiling 之上加入 4×4 register micro-kernel。16 個 local accumulator（`c00..c33`）在整個 k-loop 中保持在 register，完全消除對 C 矩陣的反覆 load/store，最後一次性 write back。

Compiler 在此結構下自動生成 ARM NEON `fmla.2d` + `ld1r.2d`（128-bit SIMD），無需手寫 intrinsics。

```
~23 GFLOPS   (BM=128, BN=128, BK=8, default)
~31 GFLOPS   (BM=512, BN=512, BK=8, best config)
```

整體加速比（vs naive baseline）：**~15x**

## Build and Run

```bash
make                                              # 預設 -O2
make CFLAGS="-std=c11 -Wall -Wextra -O3 -march=native"   # release build

./matmul                                          # naive, N=1024, 5 repeats
./matmul --kernel rowmajor 1024 5
./matmul --kernel block --bm 128 --bk 8 --bn 128 1024 5
./matmul --kernel regblock 1024 5
./matmul --kernel regblock --bm 512 --bk 8 --bn 512 1024 5

./matmul --kernel regblock --verify 1024 3        # correctness check
```

可用 `--kernel` 切換：`naive` | `rowmajor` | `block` | `regblock`  
可用 `--bm`、`--bk`、`--bn` 覆蓋 tile 大小（block / regblock kernel）  
可用 `--verify` 對照 naive reference 驗證數值正確性

## Performance Summary (Apple Silicon MacBook Air M3)

| Phase | Kernel | GFLOPS (best) |
|-------|--------|---------------|
| 0 | naive (i-j-k) | ~2.1 |
| 1 | rowmajor (i-k-j) | ~14.9 |
| 2 | cache-blocked | ~16.9 |
| 3 | register-blocked (4×4) | ~30.8 |

> 數值為實測快照，環境：macOS、Clang、`-O3 -march=native`、N=1024。

## Research Reports

- `research/report1.md` — Phase 0 naive baseline 問題紀錄
- `research/report2.md` — Phase 1 loop reorder 觀察
- `research/report3.md` — Phase 2 cache blocking 分析（BK/BM/BN sweep）
- `research/report4.md` — Phase 3 register blocking 分析（micro-kernel 設計、NEON 向量化確認）
