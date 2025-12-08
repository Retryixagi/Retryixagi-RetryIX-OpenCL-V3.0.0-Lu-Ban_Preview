
# RetryIX v3.0.0-preview “Lu Ban” (魯班) — Full Bilingual README

> **License: MIT License｜MIT 授權**  
> Released under the MIT License. Free for open-source and commercial use with attribution.  
> 本專案採 MIT 授權，可自由使用於開源與商業用途，需保留版權宣告。

---

# 🇹🇼 中文版本  
# 🇺🇸 English Version

> **格式說明：**  
> **每個章節皆為 中英對照｜English follows Chinese**

---

## 🔥 概要｜Overview

**中文：**  
RetryIX v3.0.0 是一套 **完全原創、非 wrapper** 的 OpenCL 風格計算系統──但底層完全自製，並未使用任何 OpenCL／CUDA／ROCm 實作。  
其目的在於：以軟體方式統一不同 GPU／異質設備，並提供超越硬體限制的功能（128-bit／256-bit 原子操作、零拷貝拓撲、多模態架構等）。

**English:**  
RetryIX v3.0.0 is a **fully original compute and memory system** inspired by OpenCL—but **not** a wrapper and **not** built on top of OpenCL, CUDA, or ROCm.  
Its goal is to unify heterogeneous compute devices across vendors while enabling **software-defined capabilities beyond hardware limits**, such as 128-bit/256-bit atomics, zero-copy network paths, and multimodal topology processing.

---

## ✨ 核心特色｜Key Features

**中文：**

- **261 個 API 導出函數（純 RetryIX）**  
- **五大拓撲類型**：Network / Audio / Multimodal / Atomic / SVM  
- **跨廠商統一接口**：CUDA／ROCm／oneAPI／Vulkan  
- **零拷貝 GPU ↔ Network 傳輸**  
- **完整 SVM（Shared Virtual Memory）記憶體引擎**  
- **軟體級 128-bit 與 256-bit 原子操作（超越硬體限制）**  

**English:**

- **261 exported API functions** (native RetryIX)  
- **Five topology classes:** Network / Audio / Multimodal / Atomic / SVM  
- **Unified interface across vendors:** CUDA, ROCm, oneAPI, Vulkan  
- **True zero-copy GPU ↔ network transfer**  
- **Full SVM memory engine with precise alignment**  
- **Software 128-bit & 256-bit atomic operations (beyond hardware limits)**  

---

## ⚠️ 重要聲明｜Important Notice

**中文：**  
RetryIX 雖採 OpenCL 風格 API，但底層完全自製，**不是 OpenCL、不是 ROCm、不是 CUDA** 的任何變體。  

**English:**  
RetryIX uses an OpenCL-style API surface, but the implementation is fully original.  
It is **not** based on OpenCL, **not** based on CUDA, and **not** derived from ROCm in any form.

---

## 📁 目錄結構｜Directory Structure

```
retryix_production/
├── src/                    # 核心源碼 / Core sources
│   ├── retryix.c          # 主入口 / main entry
│   ├── retryix_compat.c   # 兼容層 / compatibility layer
│   ├── core/              # 核心 API / Core APIs
│   ├── device/            # 設備管理 / Device manager
│   ├── host/              # Host 端 / Host-side logic
│   ├── kernel/            # Kernel 管理 / Kernel manager
│   ├── memory/            # 記憶體系統 / Memory engine
│   ├── svm/               # SVM + 原子操作 / SVM & Atomics
│   └── comm/              # 通訊／零拷貝 / Zero-copy & Comm
├── include/               # 標頭文件 / Header files
├── examples/              # 示例程式 / Examples
├── lib/                   # 靜態／動態庫 / Libraries
├── bin/                   # 測試與工具 / Binaries & tests
└── build_modular.bat      # MSVC 編譯腳本（無需 OpenCL）/ Build script
```

**中文：** 17 個源碼 + 17 個標頭 → 生產級乾淨代碼庫  
**English:** 17 source files + 17 headers → clean production-ready codebase

---

## 🚀 快速開始｜Quick Start

### 1. 編譯示例程式｜Build Examples
```cmd
build_clean.bat
```

### 2. 編譯 RetryIX 主庫（無需 OpenCL）｜Build Full RetryIX Library (No OpenCL Needed)
```cmd
build_modular.bat
```

### 3. 執行測試｜Run Tests
```cmd
bin\test_256bit_atomic.exe
bin\stress_test_256bit_atomic.exe
bin\stress_test_128bit_atomic.exe
```

**English:** All tests expected to pass:
- 128-bit: 17–18M ops/s  
- 256-bit: 19–23M ops/s  
- 64-thread concurrency: zero corruption  

---

## 📚 核心功能說明｜Core Functionality

### 1️⃣ SVM 記憶體系統｜SVM Memory Engine

**中文：**
- 完整 Shared Virtual Memory  
- 256-byte alignment  
- 8GB 虛擬裝置空間  
- 追蹤分配／釋放／統計  

**English:**
- Full Shared Virtual Memory  
- 256-byte alignment  
- 8GB virtual device address space  
- Full allocation/free tracking  

---

### 2️⃣ 軟體級原子操作｜Software Atomics

| Bit Width | CUDA HW | RetryIX |
|----------:|:-------:|:-------:|
| 32        | Yes     | Yes     |
| 64        | Yes     | Yes     |
| 128       | No      | **Yes (software atomic)** |
| 256       | No      | **Yes (pair-CAS)** |

**中文：** 全部具備順序一致性（Sequential Consistency）。  
**English:** All atomics guarantee Sequential Consistency.

---

### 3️⃣ 快／慢路徑切換｜Fast/Slow Path Switching

**中文：**
- 快路徑：CPU 原生指令（如 cmpxchg16b）  
- 慢路徑：跨平台 spinlock  
- 自動 fallback  

**English:**
- Fast path: native CPU intrinsics  
- Slow path: portable spinlock  
- Automatic fallback  

---

### 🧭 CPU 能力與執行條件｜CPU capabilities & runtime requirements

**中文（簡述）：**
- 快路徑（高效能）依賴 CPU 對 128-bit 原子比較交換的原生支援（x86_64 平台為 CMPXCHG16B；ARM 平台則需 ARMv8.x 的 128-bit CAS 指令／pair-CAS 支援，例如 LDAXP/STXP、CASP）。
- 若目標處理器沒有這類原生 128-bit CAS，RetryIX 會自動回退到安全但較慢的軟體實作（spinlock / pair-CAS 軟體路徑），功能仍可用但效能會降低。
- 256-bit 運算目前沒有通用的單指令硬體支援；RetryIX 以 pair-CAS / 軟體原子實作來正確實現 256-bit 原子語意。
- 建議：若要達到最佳效能，請在 x86_64 平台使用支援 CMPXCHG16B 的現代 CPU（大多數 Intel/AMD x64 CPU 都支援），或在 ARM 平台選擇支援 128-bit CAS 的 ARMv8.1+ / v8.2+ CPU。

**English (short):**
- The fast path (high-throughput) requires native 128-bit compare-and-swap support on the CPU. On x86_64 this is CMPXCHG16B; on ARM it requires 128-bit CAS/pair-CAS support (LDAXP/STXP or CASP available in newer ARMv8.x CPUs).
- If the CPU lacks native 128-bit CAS, RetryIX automatically falls back to a safe software implementation (spinlock or software pair-CAS), which preserves correctness but runs slower.
- There is no single, widely-available hardware 256-bit CAS; RetryIX implements correct 256-bit atomics via pair‑CAS and software fallback.
- Recommendation: for peak performance use modern x86_64 CPUs with CMPXCHG16B (most Intel/AMD 64-bit processors) or ARMv8.1+/v8.2+ parts that expose 128-bit CAS support. Compiler toolchains (GCC/Clang/MSVC) provide matching intrinsics/lowering (e.g., __atomic_compare_exchange_n or _InterlockedCompareExchange128).

---

### 4️⃣ GPU ↔ Network 零拷貝｜Zero-Copy GPU ↔ Network

**中文：**
- GPU buffer 可直接映射至 socket buffer（無 CPU 中轉）  
- 適合 AI 推論、分散式系統與高頻通訊場景  

**English:**
- GPU buffers can be mapped directly to socket buffers (no CPU copy)  
- Ideal for inference, distributed memory, and high-throughput networking

---

## 💻 使用示例｜Usage Example

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

**中文：** 完整支援 128-bit 自增。  
**English:** Fully supports 128-bit atomic add.

---

## 📊 測試摘要｜Test Summary

**中文：**
- 所有測試完全通過  
- 64 線程：零腐敗  

**English:**
- All tests pass  
- Zero corruption under 64-thread stress  

---

## 🛠️ 技術細節｜Technical Details

**中文：**
- 128-bit：16-byte 對齊  
- 256-bit：雙 128-bit 對齊  
- Thread-safe：Windows CriticalSection／POSIX spinlock  
- 支援 Windows

**English:**
- 128-bit: 16-byte aligned  
- 256-bit: dual 128-bit alignment  
- Thread-safe: Windows CriticalSection / POSIX spinlock  
- Supports Windows 

---

## 📝 版本歷史｜Version History

### v3.0.0
- 新增 128/256-bit 原子  
- 全自動路徑切換  
- 完整壓力測試套件  

### v2.x
- 初版 SVM  
- 32/64-bit 原子  

---

## 📄 授權條款｜License

**中文：** 本專案採 MIT 授權，可自由商用，請保留授權聲明。  
**English:** Licensed under the MIT License. Please retain the license header.

---

**RetryIX v3.0 — Software-defined GPU capability beyond all hardware limits.**  
**RetryIX v3.0 — 軟體定義 GPU，突破所有硬體上限。**
ined GPU capability beyond hardware
limits.**
bat            # full library
```

### Run Tests
All tests expected to pass with 17–23M ops/sec.

### License
MIT License.

---

**RetryIX v3.0 — Software-defined GPU capability beyond hardware limits.**

t            # full library
```

### Run Tests
All tests expected to pass with 17–23M ops/sec.

### License
MIT License.

---

**RetryIX v3.0 — Software-defined GPU capability beyond hardware limits.**e.org/licenses/MIT

---


## 🔗 相關文件 | Related Files

- `CHANGELOG_V7.md`: 詳細更新日誌 | Changelog
- `PYTORCH_INTEGRATION_FINAL_REPORT.md`: PyTorch 集成報告 | PyTorch integration
- `README_RETRYIX_V7.md`: v7 版本說明 | v7 release notes

---


**RetryIX v3.0.0** - 超越硬體限制的 GPU 模擬層 | GPU abstraction beyond hardware limits 🚀
