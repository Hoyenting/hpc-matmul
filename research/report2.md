# Report 2: Phase 0 Baseline Harness 強化與語意統一紀錄

日期：2026-02-23  
環境：Apple Silicon MacBook Air M3 / macOS / `cc` (`-std=c11 -Wall -Wextra -O2`)  
範圍：`main.c` + `src/matmul_naive.c` + `src/matmul_naive_rowmajor.c`

## 本階段量測快照

測試指令：

```bash
./matmul --verify --kernel naive 512 5
./matmul --verify --kernel rowmajor 512 5
```

觀察值（一次快照）：

- 比較原則：`naive` 與 `rowmajor` 應使用相同輸入規模與相同 repeats（例如 `M=N=K=512`, `repeats=5`）進行對照。
- `naive(512^3, repeats=5)`: `min_time = 0.223510 s`, `median_time = 0.228942 s`, `mean_time = 0.228291 s`, `stddev_time = 0.002530 s`
- `naive(512^3, repeats=5)`: `gflops_best = 1.20`, `gflops_median = 1.17`, `gflops_mean = 1.18`
- `rowmajor(512^3, repeats=5)`: `min_time = 0.031782 s`, `median_time = 0.032037 s`, `mean_time = 0.032162 s`, `stddev_time = 0.000424 s`
- `rowmajor(512^3, repeats=5)`: `gflops_best = 8.45`, `gflops_median = 8.38`, `gflops_mean = 8.35`
- `verify (兩者) = PASS`, `max_abs_err = 0.000e+00`, `max_rel_err = 0.000e+00`

## 實驗設定（可重現資訊）

- 問題規模：建議比較快照使用相同條件，例如 `M=N=K=512`
- repeats：建議比較快照使用相同條件，例如 `5`
- 精度：`double`
- 編譯參數：`CFLAGS="-std=c11 -Wall -Wextra -O2"`（預設）
- 資料初始化方式：固定 seed 隨機初始化（A/B 分別使用固定 seed）
- 計時方法：`mach_absolute_time()` + 1 次 warm-up（不納入統計）

## 本階段主要發現

1. `i-k-j` row-major kernel 顯著改善 baseline locality

- 現象：新增 `matmul_naive_rowmajor` 後，在相同 `A/B` 與同一 benchmark harness 下，GFLOPS 顯著高於 `i-j-k` naive。
- 影響：建立了更有代表性的「仍屬 naive 層級但 locality 較佳」比較組，便於後續與 blocking/SIMD 版本做階段比較。
- 證據：`./matmul --kernel rowmajor ...` 的 `gflops_median/gflops_mean` 明顯高於 `./matmul --kernel naive ...`（同等條件下）。

2. Kernel 語意已統一為 `C = A * B`，比較公平性提升

- 現象：`naive` 與 `rowmajor` 現在都由 kernel 自行完整寫滿 `C`，不依賴 `main` 在每回合先清零。
- 影響：消除了「部分 kernel 依賴外部初始化」造成的 API 語意不一致與比較偏差，後續新增 kernel 可沿用同一規範。
- 證據：`rowmajor` kernel 在每個 `i` 內先清 `c_row`，`main.c` 已移除每回合 `memset(C,0,...)`。

3. Correctness verify 機制補上了性能實驗的正確性 gate

- 現象：新增 `--verify` 與 `--tol`，可用 `naive` 作 reference 檢查被測 kernel 的 `C_test`。
- 影響：後續優化版本不再只看 GFLOPS，可在同一命令下同時檢查 correctness，降低「快但算錯」風險。
- 證據：目前 `rowmajor` 在 `--verify` 下可得到 `PASS`，並輸出 `max_abs_err` / `max_rel_err`。

## 問題清單（Issue Tracker）

1. `naive_rowmajor` 之 `C` row clear 成本應納入計時，以維持演算法層級比較公平性

- 優先級：`Medium`
- 狀態：`Resolved`
- 可能原因：`rowmajor(i-k-j)` 採用 `+=` 累加，為滿足 kernel 語意 `C = A*B`，必須在 kernel 內對每列 `C` 進行初始化（clear）。
- 暫定解法：曾考慮將 clear 動作移出計時區間，但基於方法學公平性，該成本屬此實作必要開銷，故應納入計時結果。

2. 尚未完成分塊、並行與向量化優化，研究結論仍限於 baseline 層級

- 優先級：`High`
- 狀態：`Open`
- 可能原因：本階段重點在建立公平比較框架與 correctness gate，尚未進入效能最佳化主體（Tiling/Blocking、Parallelism、SIMD）。
- 暫定解法：下一階段依序導入分塊優化、並行化與向量化，並沿用現有 `--kernel` / `--verify` / 統計輸出機制做量化比較。

## 本階段結論

- 已完成 baseline harness 的第二階段強化：加入 `i-k-j` kernel、kernel 語意統一、correctness verify。
- 目前主要瓶頸仍是計算核心本身尚未進入 blocking/SIMD/parallel 最佳化。
- 可以進入下一階段（block/tiling）研究，且已有較完整的公平比較與正確性基礎。

## 下一階段行動

1. 新增 blocked/tiling kernel，沿用既有 `--kernel` 與 `--verify` 流程比較。
2. 導入 parallelism（多執行緒）版本，評估在相同 correctness 條件下的加速效果。
3. 導入 SIMD（向量化）版本，分離向量化與快取最佳化的效益貢獻。

## 附錄

- 相關程式版本：`working tree (Phase 0 baseline + rowmajor + verify)`
- 結果檔案：尚未落盤（目前為 terminal output）
- 參考資料：本專案 `README.md`、`research/report1.md`
