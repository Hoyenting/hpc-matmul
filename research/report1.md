# Report 1: Phase 0 Naive Baseline 問題紀錄

日期：2026-02-19  
環境：Apple Silicon MacBook Air M3  
範圍：`main.c` + `src/matmul_naive.c`（double precision, row-major, i-j-k）

## 本階段量測快照

測試指令：

```bash
./matmul 1024 3
```

觀察值（一次快照）：

- `best_time = 1.956911 s`
- `avg_time = 2.042982 s`
- `best = 1.10 GFLOPS`
- `avg = 1.05 GFLOPS`

## 目前發現的問題

1. 計算效能明顯偏低

- Naive `i-j-k` 在 `B[k * N + j]` 的存取上具有跨列（stride）特性，cache locality 差。
- 目前 GFLOPS 僅作為 baseline 參考，與硬體峰值有顯著落差屬預期現象。

2. 單次快照不足以代表穩定表現

- 目前只記錄一次實跑輸出，尚未建立統計分布（多輪批次、平均、標準差、離群值）。
- 後續若做版本比較，可能因背景負載或溫度狀態造成偏差。

3. 尚未建立 correctness gate

- 目前有 `checksum_guard` 防止優化移除計算，但尚未有 reference 比對機制。
- 後續優化版本若只看 GFLOPS，存在「快但算錯」風險。

4. 研究資料輸出格式尚未標準化

- 目前結果主要在終端輸出，尚未自動寫入固定欄位檔案（CSV/JSON）。
- 後續資料整理與繪圖會增加人工成本與出錯風險。

## 造成原因（初步）

- Baseline 設計優先「可理解與可重現」，未引入 cache blocking、SIMD、parallel。
- 缺乏批次實驗腳本與統一資料管線，研究節奏偏手動。

## 後續行動（下一階段）

1. 建立 correctness test（對照 reference 實作 + 誤差門檻）。
2. 導入 blocked/tiling 版本，評估 locality 改善幅度。
3. 導入 SIMD（NEON）版本，分離向量化帶來的效益。
4. 建立 `results/` 標準輸出格式（至少包含 M/N/K、repeats、time、GFLOPS、版本標籤、時間戳）。
