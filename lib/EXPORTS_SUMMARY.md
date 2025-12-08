# RetryIX 函數導出摘要

**生成日期**: 2025年12月08日  
**DLL 版本**: retryix.dll (模塊化生產版)  
**總函數數**: 292

---

## 函數分類統計

### 1. cJSON/cJSONUtils (92 函數)
- **cJSON 核心**: 48 函數 (JSON 解析、創建、操作)
- **cJSONUtils**: 30 函數 (JSON Patch、Merge、Pointer 操作)

### 2. RetryIX 核心函數 (167 函數)

#### 2.1 原子操作 (Atomic Operations) - 36 函數
- `retryix_atomic_compare_exchange_*` - CAS 操作 (32/64/128/256-bit)
- `retryix_atomic_exchange_*` - 原子交換
- `retryix_atomic_fetch_add/sub/and/or/xor_*` - 原子算術/位運算
- `retryix_atomic_fetch_min/max_*` - 原子最小/最大值

#### 2.2 拓撲發現 (Topology Discovery) - 5 函數
- `retryix_discover_atomic_topology_json()` - 原子操作拓撲
- `retryix_discover_audio_topology_json()` - 音訊設備拓撲
- `retryix_discover_multimodal_topology_json()` - 多模態計算拓撲
- `retryix_discover_network_topology_json()` - 網絡拓撲
- `retryix_discover_svm_topology_json()` - SVM 記憶體拓撲

#### 2.3 設備管理 (Device Management) - 28 函數
- `retryix_discover_all_devices()` - 設備發現
- `retryix_enumerate_devices()` - 設備枚舉
- `retryix_device_*` - 設備查詢與配置
- `retryix_query_*` - 設備狀態查詢
- `retryix_select_best_device()` - 最佳設備選擇

#### 2.4 SVM 記憶體管理 (SVM Memory) - 24 函數
- `retryix_svm_alloc/free()` - SVM 記憶體分配/釋放
- `retryix_svm_create/destroy_context()` - SVM 上下文管理
- `retryix_svm_atomic_*` - SVM 原子操作
- `retryix_mem_*` - 記憶體建議、預取、綁定

#### 2.5 內核執行 (Kernel Execution) - 12 函數
- `retryix_kernel_create_from_source()` - 內核創建
- `retryix_kernel_execute/execute_1d()` - 內核執行
- `retryix_kernel_set_*_arg()` - 參數設置
- `retryix_kernel_wait_all()` - 同步等待

#### 2.6 ZeroCopy 網絡 (ZeroCopy Network) - 32 函數
- `retryix_zerocopy_net_init/cleanup()` - 網絡初始化
- `retryix_zerocopy_gpu_to_net/net_to_gpu()` - GPU-網絡零拷貝
- `retryix_zerocopy_configure_rdma/dpdk()` - RDMA/DPDK 配置
- `retryix_zerocopy_*_init()` - InfiniBand/iWARP/RoCE/OmniPath 初始化
- `retryix_zerocopy_dma_*` - DMA 傳輸管理

#### 2.7 系統與工具 (System & Utils) - 30 函數
- `retryix_initialize/cleanup()` - 系統初始化
- `retryix_get_error_string/version()` - 錯誤處理與版本查詢
- `retryix_*_json()` - JSON 導出功能
- `retryix_memory_*` - 記憶體統計與驗證
- `retryix_send_command()` - 命令發送

### 3. 其他工具函數 (16 函數)
- `get_safe_strcpy()` - 安全字符串複製
- `retryix_free_json()` - JSON 記憶體釋放

---

## 新增函數亮點 (相比 v1.0)

### ✨ 完整拓撲發現系統
5 個新的 JSON 拓撲發現函數，提供：
- 網絡拓撲 (4 物理 NIC，過濾 5 虛擬 NIC)
- 音訊拓撲 (完整 Unicode 設備名稱)
- 多模態拓撲 (設備分類：CPU/GPU/Accelerator)
- 原子拓撲 (32-256 bit 原子操作能力)
- SVM 拓撲 (真實 NUMA 檢測，記憶體層級)

### ✨ 增強原子操作
- 新增 128/256-bit CAS 操作
- 支援浮點數原子操作 (f32/f64)
- 新增原子最小/最大值操作

### ✨ cJSONUtils 集成
- 完整 JSON Patch (RFC 6902) 支持
- JSON Merge Patch (RFC 7396) 支持
- JSON Pointer (RFC 6901) 操作

---

## 使用範例

### 拓撲發現
```c
// 發現多模態拓撲
const char* json = retryix_discover_multimodal_topology_json();
// 返回分類設備: cpu_devices, gpu_devices, accelerator_devices

// 發現 SVM 記憶體拓撲
const char* svm_json = retryix_discover_svm_topology_json();
// 返回 NUMA 配置、記憶體層級、SVM 能力

retryix_free_json(json);
retryix_free_json(svm_json);
```

### 原子操作
```c
// 128-bit CAS
__uint128_t expected = 0, desired = 1;
bool success = retryix_atomic_compare_exchange_u128(ptr, &expected, desired);

// 256-bit pair CAS
struct pair_256 { __uint128_t low, high; };
retryix_atomic_compare_exchange_pair_256(ptr, &expected_pair, &desired_pair);
```

### ZeroCopy 網絡
```c
// GPU → 網絡零拷貝
void* gpu_buffer = ...; // GPU 記憶體
retryix_zerocopy_gpu_to_net(gpu_buffer, size, dest_ip, dest_port);

// 網絡 → GPU 零拷貝
retryix_zerocopy_net_to_gpu(src_ip, src_port, gpu_buffer, size);
```

---

## 技術細節

### 設備分類 (Multimodal Topology)
- **CPU Devices**: 計算密集型工作負載
- **GPU Devices**: 並行計算、圖形渲染
- **Accelerator Devices**: 專用加速器 (FPGA, ASIC, TPU)

### NUMA 檢測 (SVM Topology)
- Windows: `GetLogicalProcessorInformationEx(RelationNumaNode)`
- 真實檢測節點數 (非硬編碼)
- UMA vs NUMA 分類
- 拓撲感知分配策略

### 音訊完整性 (Audio Topology)
- Unicode API: `waveOutGetDevCapsW()`
- UTF-8 轉換支援國際字符
- 完整設備名稱 (不截斷)

---

## 檔案信息

- **完整導出表 (dumpbin)**: `lib/retryix_exports_dumpbin2.txt` (292 exports)
- **DLL 位置**: `lib/retryix.dll`
- **導入庫**: `lib/libretryix.dll.a` (MinGW), `lib/retryix.lib` (MSVC)
- **備份**: `lib/exports_full_backup.txt` (舊版 192 函數)

---

**[Lu Ban @ 2025-12-08]** - 模塊化生產版本，完整拓撲能力，292 個導出函數
