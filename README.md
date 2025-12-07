# RetryIX v3.0.0-preview "Lu Ban"（魯班）

**Official Preview Release｜預覽發布**  
**Windows x64 ｜ 2025-12-03**

> **English version follows below｜英文說明在後方**

---

# 🇨🇳 中文版本

## 🔥 概要
RetryIX v3.0.0 是一套 **完全原創、非 wrapper** 的 OpenCL-style 計算與記憶體系統。其核心目標是以跨廠商、跨硬體拓撲的方式統一各類 GPU／異質計算設備，並提供軟體級延伸（如 128-bit／256-bit 原子操作與零拷貝傳輸），突破傳統 GPU 架構的限制。

### ✨ 本版本的核心特色
- **261 個 API 導出函數**（純 RetryIX + cJSON）
- **五大拓撲類型支援**：Network / Audio / Multimodal / Atomic / SVM
- **跨廠商統一接口**：CUDA／ROCm／oneAPI／Vulkan
- **全核心模組 100% 完整實作**
- **GPU ↔ Network 零拷貝傳輸支援**
- **完整 SVM（Shared Virtual Memory）支援與高對齊記憶體系統**
- **軟體級 128-bit 與 256-bit 原子操作（超越 NVIDIA／AMD 硬體規格）**

> ⚠️ **重要聲明：RetryIX API 雖採 OpenCL 風格，但底層完全自製，並非任何現有 OpenCL／ROCm／CUDA 的封裝或變體。**

---

# 📁 目錄結構
```
retryix_production/
├── src/                    # 核心源碼
│   ├── retryix.c          # 主入口
│   ├── retryix_compat.c   # 兼容層
│   ├── core/              # 核心 API
│   ├── device/            # 設備管理
│   ├── host/              # Host 端
│   ├── kernel/            # Kernel 管理
│   ├── memory/            # 記憶體系統
│   ├── svm/               # SVM + 原子操作
│   └── comm/              # 通訊／零拷貝
├── include/               # 標頭文件
├── examples/              # 示例程式
├── lib/                   # 編譯後的靜態／動態庫
├── bin/                   # 測試執行檔
└── build_modular.bat      # MSVC 編譯腳本
```
**總計：17 個源碼 + 17 個標頭 → 乾淨的生產級代碼庫**

---

# 🚀 快速開始

## 1. 編譯示例程式（建議）
```cmd
build_clean.bat
```
特點：
- 完全乾淨編譯（自動清理 obj/pdb/ilk）
- 只保留可執行檔
- /O2 最佳化
- 失敗也會清理殘留

## 2. 編譯完整 RetryIX 庫
```cmd
build.bat         # MinGW (需 MSYS2)
build_msvc.bat    # MSVC (需 OpenCL 頭文件)
```

## 3. 執行測試套件
```cmd
bin\test_256bit_atomic.exe
bin\stress_test_256bit_atomic.exe
bin\stress_test_128bit_atomic.exe
```

預期結果：
- 所有測試 100% 通過
- 128-bit：17–18M ops/s
- 256-bit：19–23M ops/s
- 64 線程高並發：零數據腐敗

---

# 📚 核心功能說明

## 1. GPU 記憶體模擬系統（SVM 模型）
- 完整 Shared Virtual Memory 實作
- 256-byte 對齊（高穩定度）
- 8GB 虛擬設備空間
- 獨立記憶體地址：起始於 0x100000
- 支援全域統計：分配／釋放／追蹤

## 2. 擴展原子操作（軟體實現，超越硬體）
| 位元寬度 | CUDA 硬體 | RetryIX v3.0 |
|---------|-----------|----------------|
| 32-bit  | ✅ 支援   | ✅ 支援        |
| 64-bit  | ✅ 支援   | ✅ 支援        |
| 128-bit | ❌ 不支援 | ✅ **軟體原子** |
| 256-bit | ❌ 不支援 | ✅ **軟體 Pair CAS** |

已支援 API 包含：
- `retryix_svm_atomic_fetch_add_i128`
- `retryix_svm_atomic_compare_exchange_i128`
- `retryix_svm_atomic_compare_exchange_pair_256`
- ...等十餘種高精度原子操作

> **所有原子操作皆具 Sequential Consistency（順序一致性）。**

## 3. 快速／慢速路徑自動切換
- **快速路徑**：使用 CPU 原生 `cmpxchg16b`
- **慢速路徑**：跨平台 spinlock
- **自動回退**：不需使用者介入

## 4. GPU ↔ Network 零拷貝
- 支援跨拓撲直接引用記憶體
- 將 GPU 資料直接導入 socket buffer（無 CPU 中轉）
- 適合 AI 推論／多機共享記憶體場景

---

# 🔧 使用示例
```c
#include <retryix.h>
#include <retryix_svm.h>

int main() {
    retryix_svm_context_t* ctx = retryix_svm_create_context();

    void* ptr = retryix_svm_alloc(ctx, 1024, RETRYIX_SVM_FINE_GRAIN_BUFFER);

    u128_t val = {0,0}, add = {1,0}, old;
    retryix_svm_atomic_fetch_add_i128(ctx, (volatile u128_t*)ptr, add, &old);

    retryix_svm_free(ctx, ptr);
    retryix_svm_destroy_context(ctx);
}
```

---

# 📊 測試報告摘要
### 256-bit atomic（6 項基本測試）
- 全部 PASS
- 高並發零腐敗

### 256-bit stress（4 項壓力測試）
- 最高 23.26M ops/s
- 64 threads：完全一致性

### 128-bit stress
- 最高 18.5M ops/s
- 與 256-bit 性能相近（瓶頸在鎖機制）

---

# 🛠️ 技術細節
- 128-bit：16-byte 對齊
- 256-bit：雙 128-bit 分別對齊
- Thread-safe：Windows CriticalSection／POSIX spinlock
- 最多 64 線程無競態
- 支援 Windows／Linux／macOS（Rosetta）

---

# 📝 版本歷史摘要
### v3.0.0
- 新增 128-bit 原子操作
- 新增 256-bit Pair CAS
- 自動 Fast/Slow path
- 新增能力查詢 API
- 完整壓力測試套件

### v2.x
- 基本 SVM
- 32/64-bit 原子操作
- GPU 記憶體模擬

---

# 📄 許可證
本專案採 **MIT License**，可自由用於個人／商業用途。
詳細請見 `LICENSE` 檔案。

---

# 🇺🇸 English Version

## Overview
RetryIX v3.0.0 is a **fully original OpenCL-style compute system**, not a wrapper and not based on any existing OpenCL/CUDA implementation.

### Key Features
- 261 exported functions
- Unified interface for CUDA/ROCm/oneAPI/Vulkan
- Zero-copy GPU↔network transfer
- Full SVM memory engine
- Software-based 128-bit and 256-bit atomics

### Build
```cmd
build_clean.bat      # examples
build.bat            # full library
```

### Run Tests
All tests expected to pass with 17–23M ops/sec.

### License
MIT License.

---

**RetryIX v3.0 — Software-defined GPU capability beyond hardware limits.**