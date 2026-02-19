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
- 三層迴圈（`i-j-k`）

### Fair Profiling Design

- 記憶體使用 `posix_memalign` 做 `128B` 對齊
- 預設測試規模為 `N=1024`（可輸入更大）
- 先做 1 次 warm-up，不納入統計
- 正式重複執行多次，輸出 `best/avg time` 與 `best/avg GFLOPS`
- 以 `checksum_guard` 防止計算被不當優化掉

## Build and Run (Minimal)

```bash
make
./matmul
./matmul 1024 3
./matmul 1024 1024 1024 3
```

備註：目前 `Makefile` 預設會編譯 `main.c` + `src/matmul_naive.c`。

## Current Baseline Snapshot (Apple Silicon MacBook Air M3)

一次實測範例（`./matmul 1024 3`）：

- `best_time = 1.956911 s`
- `avg_time = 2.042982 s`
- `best = 1.10 GFLOPS`
- `avg = 1.05 GFLOPS`

> 此數值為當下環境快照，用於階段比較；正式報告建議記錄多次 run 與統計分布。
