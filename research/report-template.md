# Report <N>: <Phase Name> 問題紀錄

日期：<YYYY-MM-DD>  
環境：<Machine / CPU / OS / Compiler>  
範圍：<涉及檔案與實作版本，例如 main.c + src/matmul_blocked.c>

## 本階段量測快照

測試指令：

```bash
<command 1>
<command 2>
```

觀察值（一次快照）：

- `best_time = <...> s`
- `avg_time = <...> s`
- `best = <...> GFLOPS`
- `avg = <...> GFLOPS`
- `error_metric = <...>`（若有 correctness 驗證）

## 實驗設定（可重現資訊）

- 問題規模：`M=<...>, N=<...>, K=<...>`
- repeats：`<...>`
- 精度：`float` 或 `double`
- 編譯參數：`<CFLAGS / LDFLAGS>`
- 資料初始化方式：`<random seed / fixed pattern>`
- 計時方法：`<timer API + warm-up 次數>`

## 本階段主要發現

1. <發現 1 標題>

- 現象：<具體觀察到什麼>
- 影響：<對效能或正確性的影響>
- 證據：<對應數據、log、圖表檔名>

2. <發現 2 標題>

- 現象：<...>
- 影響：<...>
- 證據：<...>

3. <發現 3 標題>

- 現象：<...>
- 影響：<...>
- 證據：<...>

## 問題清單（Issue Tracker）

1. <問題描述>

- 優先級：`High / Medium / Low`
- 狀態：`Open / In Progress / Resolved`
- 可能原因：<...>
- 暫定解法：<...>

2. <問題描述>

- 優先級：`High / Medium / Low`
- 狀態：`Open / In Progress / Resolved`
- 可能原因：<...>
- 暫定解法：<...>

## 本階段結論

- <一句話總結本階段進展>
- <一句話總結主要瓶頸>
- <一句話說明目前是否可進入下一階段>

## 下一階段行動

1. <next step 1>
2. <next step 2>
3. <next step 3>

## 附錄

- 相關程式版本：`<commit hash / branch>`
- 結果檔案：`results/<file1>`, `results/<file2>`
- 參考資料：<paper / blog / notes>
