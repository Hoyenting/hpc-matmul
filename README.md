# hpc-matmul

矩陣乘法最佳化與 profiling 專案，目標是以可重現、可比較的方式，逐階段從 baseline 推進到進階優化版本。

## README Scope

本文件聚焦「成果展示」與「實驗方法」。  
階段性問題與研究觀察請見：`research/report1.md`。

## Repository Structure

- `main.c`: baseline benchmarking harness（隨機初始化、計時、GFLOPS 輸出）
- `src/matmul_naive.c`: Phase 0 參考實作（3 層迴圈 naive matmul）
- `bench/`: 後續 benchmarking scripts
- `tests/`: correctness tests
- `results/`: 實驗輸出（建議放 CSV / log）
- `scripts/`: 輔助腳本
- `research/`: 研究紀錄與問題追蹤

## Phase 0: Naive Baseline (Current Milestone)

### Milestone Objective

建立可重現、可比較的 baseline，作為後續 blocking / SIMD / parallel 優化的對照組。

### Kernel

- `C[MxN] = A[MxK] * B[KxN]`
- row-major layout
- `double` precision
- 目前可切換 baseline kernel：
  - `naive`（`i-j-k`）
  - `rowmajor`（`i-k-j`，較佳 row-major locality）

### Fair Profiling Design

- 記憶體使用 `posix_memalign` 做 `128B` 對齊
- 預設測試規模為 `N=1024`（可輸入更大）
- 先做 1 次 warm-up，不納入統計
- kernel 語意統一為 `C = A * B`（kernel 內部完整寫滿 `C`）
- 正式重複執行多次，輸出 `min/median/mean/stddev` 時間統計
- GFLOPS 依時間統計計算（至少 `gflops_median`, `gflops_mean`）
- 以 `checksum_guard` 防止計算被不當優化掉

### CLI Parameters (Benchmark / Verify)

- `--kernel naive|rowmajor`: 切換被測 kernel
- `--verify`: 啟用 correctness 驗證（reference 使用 `naive`）
- `--tol <value>`: 設定誤差容忍（`double`，預設 `1e-9`）

輸入形式：

- `./matmul`
- `./matmul N [repeats]`
- `./matmul M N K [repeats]`
- 以上形式可與 `--kernel`、`--verify`、`--tol` 組合使用

### Output Fields (Current)

- `min_time`: repeats 中最小時間（秒）
- `median_time`: repeats 的中位數時間（秒）
- `mean_time`: repeats 的平均時間（秒）
- `stddev_time`: repeats 的標準差（秒）
- `gflops_best`: 以 `min_time` 計算的 GFLOPS
- `gflops_median`: 以 `median_time` 計算的 GFLOPS
- `gflops_mean`: 以 `mean_time` 計算的 GFLOPS
- `checksum_guard`: 防止編譯器不當移除計算
- `verify=PASS/FAIL`, `max_abs_err`, `max_rel_err`（啟用 `--verify` 時）

## Build and Run (Minimal)

```bash
make
./matmul
./matmul --kernel rowmajor 1024 5
./matmul --verify --kernel rowmajor --tol 1e-9 1024 3
./matmul 1024 3
./matmul 1024 1024 1024 3
```

備註：目前 `Makefile` 預設會編譯 `main.c`、`src/matmul_naive.c`、`src/matmul_naive_rowmajor.c`。

## Current Baseline Snapshot (Apple Silicon MacBook Air M3)

一次實測範例（`./matmul 1024 3`）：

- `min_time ≈ 1.96 s`
- `median_time / mean_time`（依 repeats 與當下環境波動）
- `gflops_best / gflops_median / gflops_mean`

> 此數值為當下環境快照，用於階段比較；正式報告建議記錄多次 run 與統計分布。
