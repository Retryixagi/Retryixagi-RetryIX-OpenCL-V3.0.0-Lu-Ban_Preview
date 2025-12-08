# RetryIX AI - 統一可視化演示系統

## 概述

這是一個統一的AI演示和可視化框架，結合了Qt圖形界面和PowerShell控制台，提供實時AI模型監控和可視化體驗。

## 設計原則

- **透明輸入** → **真實計算** → **可驗證輸出**
- **一鍵啟動** → **實時可視化** → **統一體驗**
- **跨平台兼容** → **零外部依賴** → **即時部署**

## 賈維斯架構映射

將RetryIX AI系統映射到類似賈維斯(Jarvis)的AI助手架構，分為三大功能層次：

```
📊 RetryIX AI → "小型賈維斯"概念圖

┌───────────────────────────────┐
│           感知層 (Perception)          │
│---------------------------------------│
│ • GUI 可視化 (PySide6/Qt6.1)           │
│ • Demo network 可視化 MLP/CNN/Transformer │
│ • 輸入數據接收 (隨機矩陣, 測試數據)     │
│ • 實時圖表監控 (Loss/Throughput/Latency) │
└───────────────────────────────┘
             │
             ▼
┌───────────────────────────────┐
│           思考層 (Cognition)         │
│---------------------------------------│
│ • 核心 AI 計算引擎                     │
│    - demo_ai_network.exe (MLP)       │
│    - test_cnn.exe (CNN)              │
│    - test_transformer.exe (Transformer)│
│ • 運算邏輯: 前向傳播, 反向傳播, 優化   │
│ • 決策推理: 損失計算, 準確度評估       │
│ • 數學核心: 矩陣運算, 激活函數, Softmax │
└───────────────────────────────┘
             │
             ▼
┌───────────────────────────────┐
│           執行層 (Action)           │
│---------------------------------------│
│ • 可操作演示應用                        │
│    - 基準性能測試                      │
│    - 實時推理演示                      │
│ • 自動化腳本協調 (start_ai_demo.ps1)   │
│ • 結果輸出與日誌記錄                   │
│ • PowerShell 進程管理與監控            │
└───────────────────────────────┘
```

### 架構說明

**🎯 感知層 (Perception) - "賈維斯的眼睛"**
- **視覺輸入**: Qt圖形界面接收用戶指令和顯示結果
- **數據監控**: 實時追蹤AI模型的性能指標
- **狀態感知**: 監控系統運行狀態和資源使用

**🧠 思考層 (Cognition) - "賈維斯的大腦"**
- **AI計算核心**: 純C語言實現的神經網路推理引擎
- **邏輯推理**: 基於數學運算的決策過程
- **學習適應**: 通過基準測試評估和優化性能

**⚡ 執行層 (Action) - "賈維斯的手和動作模組"**
- **任務執行**: 運行具體的AI演示和測試
- **自動化協調**: PowerShell腳本管理多進程工作流
- **結果呈現**: 格式化輸出和持久化記錄

### 擴展路徑

要讓系統更接近完整賈維斯，可以添加：

- **語音交互**: 語音輸入/輸出模組
- **任務排程**: 自動化任務調度系統
- **外部集成**: API連接和雲服務
- **學習適應**: 基於使用模式的自我優化

## 系統組件

### AI演示程序 (C語言實現)
- `demo_ai_network.exe` - 多層感知器(MLP)網路演示
- `test_cnn.exe` - 卷積神經網路(CNN)演示
- `test_transformer.exe` - Transformer架構演示
- `test_multimodal_topology.exe` - 多模態拓樸函數演示

### 可視化框架
- `visualizer.py` - PySide6/Qt6實時圖表可視化
- `start_ai_demo.ps1` - PowerShell統一啟動器

### 拓樸數據文件
- `Ackley Function_topology.csv` - Ackley函數3D地形數據
- `Rastrigin Function_topology.csv` - Rastrigin函數3D地形數據
- `Rosenbrock Function_topology.csv` - Rosenbrock函數3D地形數據
- `Multimodal Peaks_topology.csv` - 自定義多峰值函數3D地形數據

## 快速開始

### 1. 編譯AI演示程序

```batch
# 使用MSVC編譯所有演示
.\build_ai.bat

# 或手動編譯
cl.exe /nologo /MT /O2 /Iinclude /DWIN32 /D_CRT_SECURE_NO_WARNINGS demo_ai_network.c /link /out:demo_ai_network.exe
cl.exe /nologo /MT /O2 /Iinclude /DWIN32 /D_CRT_SECURE_NO_WARNINGS test_cnn.c /link /out:test_cnn.exe
cl.exe /nologo /MT /O2 /Iinclude /DWIN32 /D_CRT_SECURE_NO_WARNINGS test_transformer.c /link /out:test_transformer.exe
cl.exe /nologo /MT /O2 /Iinclude /DWIN32 /D_CRT_SECURE_NO_WARNINGS test_multimodal_topology.c /link /out:test_multimodal_topology.exe
```

### 2. 安裝Python依賴

```bash
pip install PySide6
```

### 3. 啟動統一系統

```powershell
# 完整系統 (帶可視化)
.\start_ai_demo.ps1

# 只運行演示 (無可視化)
.\start_ai_demo.ps1 -NoVisualizer

# 清理舊日誌後啟動
.\start_ai_demo.ps1 -CleanLogs
```

## 多模態拓樸演示特色

多模態拓樸函數演示是RetryIX AI系統中的一個特殊組件，專門用於探索和可視化多模態優化問題：

### 實現的函數
- **Ackley函數**: 經典多模態測試函數，具有眾多局部最優
- **Rastrigin函數**: 軸對稱多模態函數，優化極具挑戰性
- **Rosenbrock函數**: 單模態但狹窄山谷，測試收斂性
- **Multimodal Peaks**: 自定義多峰值地形，展示複雜拓樸結構

### 功能特點
- **3D地形生成**: 為每個函數生成50x50網格的拓樸數據
- **多起始點優化**: 從5個隨機位置開始梯度下降，測試多模態行為
- **實時優化追蹤**: 記錄loss、position_x、position_y的變化
- **拓樸分析**: 展示多模態函數的固有挑戰和特性

### 應用價值
- 理解多模態優化的複雜性
- 測試優化算法的魯棒性
- 可視化高維優化問題的幾何結構
- 為機器學習超參數優化提供洞見

## 可視化功能

### 實時圖表
- **Loss Curves**: 顯示訓練損失隨時間變化
- **Throughput**: 推理速度 (samples/sec)
- **Latency**: 平均延遲時間 (ms)

### 模型比較
- 同時顯示MLP、CNN、Transformer、Multimodal的性能對比
- 顏色編碼: 紅色(MLP)、藍色(CNN)、綠色(Transformer)、洋紅色(Multimodal)
- 動態更新，每秒刷新

### 控制台輸出
- 詳細的AI計算過程
- 隨機輸入驗證
- 性能基準測試結果
- 實時狀態監控

## 技術特點

### C語言AI實現
- 零外部依賴
- 純手工實現的神經網路
- 透明的數學計算
- 可驗證的隨機性

### 跨語言集成
- C程序生成CSV日誌
- Python讀取並可視化
- PowerShell協調進程
- 鬆耦合架構

### 實時監控
- 文件輪詢更新圖表
- 多線程進程管理
- 異常處理和清理
- 資源自動釋放

## 系統架構

```
┌─────────────────┐    ┌──────────────────┐
│   PowerShell    │────│   Qt Window      │
│   Launcher      │    │   (PySide6)      │
└─────────┬───────┘    └─────────┬────────┘
          │                      │
          └─────────┬────────────┘
                    │
          ┌─────────▼────────────┐
          │   CSV Log Files      │
          │   (Real-time)        │
          └─────────┬────────────┘
                    │
          ┌─────────▼────────────┐
          │   AI Demos (C)       │
          │   MLP/CNN/Transformer│
          └──────────────────────┘
```

## 使用說明

### 啟動流程
1. 腳本檢查環境依賴
2. 啟動Qt可視化窗口
3. 並行啟動三個AI演示
4. 實時監控進程狀態
5. 演示完成後自動清理

### 控制操作
- **Ctrl+C**: 停止所有演示
- **窗口關閉**: 自動終止所有進程
- **狀態監控**: 控制台顯示實時信息

### 故障排除
- 確保MSVC編譯器可用
- 確認Python和PySide6安裝
- 檢查防火牆不阻擋子進程
- 日誌文件權限正常

## 性能基準

### 測試環境
- CPU: Intel/AMD x64
- 編譯器: MSVC 19.44
- Python: 3.11+
- GUI: PySide6/Qt6.1

### 典型性能
- MLP: ~1000 samples/sec
- CNN: ~50 samples/sec
- Transformer: ~5 samples/sec

## 擴展開發

### 添加新模型
1. 實現C程序並添加logging
2. 更新PowerShell啟動器
3. 修改可視化器支持新數據類型

### 自定義可視化
- 修改`visualizer.py`添加新圖表
- 調整顏色主題和樣式
- 添加更多性能指標

## 許可協議

本項目遵循透明AI原則，鼓勵學習和修改。所有代碼均為教育目的設計。

---

**RetryIX AI** - 讓C語言證明AI無限可能！ 🚀