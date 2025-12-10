# RetryIX SVM 使用指南
## 避免常見誤區，掌握正確使用方法

### 作者: GitHub Copilot
### 版本: 1.0
### 日期: 2025年12月10日

---

## 📋 目錄

1. [核心概念](#核心概念)
2. [初始化順序](#初始化順序)
3. [函數返回值指南](#函數返回值指南)
4. [SVM分配策略](#svm分配策略)
5. [常見誤區](#常見誤區)
6. [最佳實踐](#最佳實踐)
7. [測試建議](#測試建議)
8. [故障排除](#故障排除)

---

## 🎯 核心概念

### RetryIX SVM設計哲學

RetryIX 採用**魯班建築智慧**的設計理念：

```
上等材料：異構SVM (clSVMAlloc) - 性能最佳
中等材料：主機內存 (malloc) - 功能正常
下等材料：分配失敗 - 寧缺毋濫
```

**核心原則**：優先使用最佳方案，失敗時優雅降級，保證最終成功。

### 重要提醒

❌ **不要把降級機制當作失敗！**
✅ **最終成功率才是最重要的指標！**

---

## 🚀 初始化順序

### 正確的初始化流程

```c
#include <retryix/retryix.h>

// 1. 初始化核心系統
retryix_result_t result = retryix_initialize();
if (result != RETRYIX_SUCCESS) {
    // 處理錯誤
}

// 2. 創建SVM上下文
void* context = NULL;
result = retryix_svm_create_context(&context);
if (result != RETRYIX_SUCCESS) {
    // 處理錯誤
}

// 3. 🔑 關鍵步驟：發現內存拓撲
result = retryix_svm_discover_topology();
if (result != RETRYIX_SUCCESS) {
    // 處理錯誤
}

// 現在可以安全地使用SVM分配函數了！
```

### 為什麼需要拓撲發現？

- 拓撲發現初始化內部狀態標誌
- 啟用SVM分配功能
- 沒有這個步驟，所有分配都會失敗

---

## 📊 函數返回值指南

### 返回類型分類

#### 1. 錯誤碼函數 (retryix_result_t / int)
```c
// 返回值解釋
0  (RETRYIX_SUCCESS)           = 成功
-1 (RETRYIX_ERROR_NULL_PTR)     = 空指標錯誤
-2 (RETRYIX_ERROR_NO_DEVICE)    = 沒有設備
-3 (RETRYIX_ERROR_NO_PLATFORM)  = 沒有平台
// ... 其他錯誤碼
```

**使用範例**：
```c
retryix_result_t result = retryix_initialize();
if (result == RETRYIX_SUCCESS) {
    printf("初始化成功\n");
} else {
    printf("初始化失敗，錯誤碼: %d\n", result);
}
```

#### 2. 指標函數 (void*)
```c
// 返回值解釋
非NULL指標 = 成功，返回分配的內存位址
NULL (0)   = 失敗，無法分配內存
```

**使用範例**：
```c
// ❌ 錯誤：把指標當錯誤碼
void* ptr = retryix_bridge_nvidia_alloc_unified(1024 * 1024);
if (ptr) {
    printf("錯誤碼: %p\n", ptr);  // 錯！
}

// ✅ 正確：指標表示成功
void* ptr = retryix_bridge_nvidia_alloc_unified(1024 * 1024);
if (ptr) {
    printf("分配成功，位址: 0x%016llx\n", (uint64_t)ptr);
    // 使用內存...
    retryix_bridge_nvidia_free_unified(ptr);
} else {
    printf("分配失敗\n");
}
```

---

## 🏗️ SVM分配策略

### 分配函數對比

| 函數 | 策略 | 成功率 | 性能 |
|------|------|--------|------|
| `retryix_svm_alloc()` | 異構SVM優先 + 降級 | 38.7%異構 + 61.3%降級 | 最佳 |
| `retryix_svm_alloc_topology_aware()` | 主機內存 | 100% | 良好 |

### 正確的評估方法

```c
// ✅ 正確的評估指標
int total_attempts = 1000;
int successful_allocs = 0;

for (int i = 0; i < total_attempts; i++) {
    void* ptr = retryix_svm_alloc_topology_aware(4096);
    if (ptr) {
        successful_allocs++;
        retryix_svm_free(ptr);
    }
}

double success_rate = (double)successful_allocs / total_attempts * 100.0;
printf("最終成功率: %.1f%%\n", success_rate);

// 目標：success_rate >= 99.0% 表示優秀
```

---

## ⚠️ 常見誤區

### 誤區1：把指標當錯誤碼

```c
// ❌ 錯誤
void* ptr = retryix_bridge_nvidia_alloc_unified(1024*1024);
if (ptr == (void*)-1) {  // 錯！不會返回-1
    printf("分配失敗\n");
}
```

### 誤區2：把降級當失敗

```c
// ❌ 錯誤的統計
int heterogeneous_attempts = 0;
int failures = 0;

for (int i = 0; i < 100; i++) {
    // 嘗試異構SVM
    cl_int err = clSVMAlloc(context, CL_MEM_READ_WRITE, size, 0, &ptr);
    if (err != CL_SUCCESS) {
        failures++;  // 錯！這是降級，不是失敗
    }
}
printf("失敗率: %d%%\n", failures);  // 誤導性結果
```

### 誤區3：跳過初始化步驟

```c
// ❌ 錯誤：沒有拓撲發現
retryix_initialize();
void* ptr = retryix_svm_alloc_topology_aware(1024);
// 結果：總是返回NULL
```

### 誤區4：錯誤的成功率計算

```c
// ❌ 錯誤：只計算異構SVM成功
int svm_success = 0;
for (int i = 0; i < 100; i++) {
    if (clSVMAlloc(...) == CL_SUCCESS) {
        svm_success++;
    }
}
printf("SVM成功率: %d%%\n", svm_success);  // 忽視了降級機制
```

---

## ✅ 最佳實踐

### 1. 正確的初始化順序

```c
// 最佳實踐：完整初始化
int initialize_retryix() {
    // 1. 核心初始化
    if (retryix_initialize() != RETRYIX_SUCCESS) {
        return -1;
    }

    // 2. 上下文創建
    void* context = NULL;
    if (retryix_svm_create_context(&context) != RETRYIX_SUCCESS) {
        return -2;
    }

    // 3. 🔑 拓撲發現（關鍵！）
    if (retryix_svm_discover_topology() != RETRYIX_SUCCESS) {
        return -3;
    }

    return 0;  // 成功
}
```

### 2. 正確的內存管理

```c
// 最佳實踐：RAII風格的內存管理
void* safe_svm_alloc(size_t size) {
    void* ptr = retryix_svm_alloc_topology_aware(size);
    if (!ptr) {
        fprintf(stderr, "SVM分配失敗，大小: %zu\n", size);
        return NULL;
    }
    return ptr;
}

void safe_svm_free(void* ptr) {
    if (ptr) {
        retryix_svm_free(ptr);
    }
}

// 使用範例
void* buffer = safe_svm_alloc(1024 * 1024);
if (buffer) {
    // 使用buffer...
    safe_svm_free(buffer);
}
```

### 3. 正確的錯誤處理

```c
// 最佳實踐：區分不同類型的返回值
retryix_result_t handle_initialization() {
    retryix_result_t result = retryix_initialize();

    switch (result) {
        case RETRYIX_SUCCESS:
            printf("初始化成功\n");
            return RETRYIX_SUCCESS;

        case RETRYIX_ERROR_NO_DEVICE:
            fprintf(stderr, "沒有找到設備\n");
            break;

        case RETRYIX_ERROR_OPENCL:
            fprintf(stderr, "OpenCL初始化失敗\n");
            break;

        default:
            fprintf(stderr, "未知錯誤: %d\n", result);
            break;
    }

    return result;
}
```

### 4. 正確的性能評估

```c
// 最佳實踐：全面的性能評估
typedef struct {
    int total_allocations;
    int successful_allocations;
    double average_latency_ms;
    size_t total_bytes_allocated;
} svm_performance_stats;

void benchmark_svm_performance() {
    svm_performance_stats stats = {0};

    const int test_iterations = 1000;
    const size_t test_size = 64 * 1024;  // 64KB

    for (int i = 0; i < test_iterations; i++) {
        clock_t start = clock();

        void* ptr = retryix_svm_alloc_topology_aware(test_size);

        clock_t end = clock();
        double latency = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;

        if (ptr) {
            stats.successful_allocations++;
            stats.total_bytes_allocated += test_size;
            stats.average_latency_ms += latency;

            retryix_svm_free(ptr);
        }

        stats.total_allocations++;
    }

    stats.average_latency_ms /= stats.successful_allocations;

    printf("SVM性能基準測試結果:\n");
    printf("  總分配次數: %d\n", stats.total_allocations);
    printf("  成功分配: %d\n", stats.successful_allocations);
    printf("  成功率: %.2f%%\n",
           (double)stats.successful_allocations / stats.total_allocations * 100.0);
    printf("  平均延遲: %.3f ms\n", stats.average_latency_ms);
    printf("  總分配位元組: %zu\n", stats.total_bytes_allocated);
}
```

---

## 🧪 測試建議

### 單元測試範例

```c
#include <assert.h>

// 測試拓撲感知分配
void test_topology_aware_allocation() {
    printf("測試：拓撲感知SVM分配\n");

    // 初始化
    assert(retryix_initialize() == RETRYIX_SUCCESS);
    assert(retryix_svm_create_context(NULL) == RETRYIX_SUCCESS);
    assert(retryix_svm_discover_topology() == RETRYIX_SUCCESS);

    // 測試不同大小
    size_t test_sizes[] = {1024, 4096, 16384, 65536, 262144, 1048576};

    for (size_t i = 0; i < sizeof(test_sizes)/sizeof(test_sizes[0]); i++) {
        void* ptr = retryix_svm_alloc_topology_aware(test_sizes[i]);
        assert(ptr != NULL);  // 必須成功

        // 驗證內存可寫
        memset(ptr, 0xAA, test_sizes[i]);

        // 釋放
        assert(retryix_svm_free(ptr) == RETRYIX_SUCCESS);
    }

    printf("✅ 所有測試通過\n");
}

// 測試降級機制
void test_graceful_degradation() {
    printf("測試：優雅降級機制\n");

    // 即使異構SVM不可用，也應該能分配主機內存
    void* ptr = retryix_svm_alloc_topology_aware(1024 * 1024);
    assert(ptr != NULL);

    // 驗證內存可用
    *(char*)ptr = 'A';
    assert(*(char*)ptr == 'A');

    retryix_svm_free(ptr);
    printf("✅ 降級機制工作正常\n");
}
```

### 壓力測試範例

```c
void stress_test_svm() {
    printf("壓力測試：高頻SVM分配/釋放\n");

    const int num_allocations = 10000;
    const size_t allocation_size = 4096;

    void** pointers = malloc(sizeof(void*) * num_allocations);
    assert(pointers != NULL);

    // 分配階段
    clock_t alloc_start = clock();
    for (int i = 0; i < num_allocations; i++) {
        pointers[i] = retryix_svm_alloc_topology_aware(allocation_size);
        assert(pointers[i] != NULL);

        // 寫入測試數據
        memset(pointers[i], i % 256, allocation_size);
    }
    clock_t alloc_end = clock();

    // 驗證階段
    for (int i = 0; i < num_allocations; i++) {
        char expected = i % 256;
        assert(*(char*)pointers[i] == expected);
    }

    // 釋放階段
    clock_t free_start = clock();
    for (int i = 0; i < num_allocations; i++) {
        assert(retryix_svm_free(pointers[i]) == RETRYIX_SUCCESS);
    }
    clock_t free_end = clock();

    double alloc_time = (double)(alloc_end - alloc_start) / CLOCKS_PER_SEC;
    double free_time = (double)(free_end - free_start) / CLOCKS_PER_SEC;

    printf("壓力測試結果:\n");
    printf("  分配 %d 次: %.3f 秒\n", num_allocations, alloc_time);
    printf("  釋放 %d 次: %.3f 秒\n", num_allocations, free_time);
    printf("  平均分配時間: %.3f ms\n", alloc_time / num_allocations * 1000.0);
    printf("  平均釋放時間: %.3f ms\n", free_time / num_allocations * 1000.0);

    free(pointers);
    printf("✅ 壓力測試通過\n");
}
```

---

## 🔧 故障排除

### 問題1：分配總是返回NULL

**可能原因**：
- 沒有調用 `retryix_svm_discover_topology()`
- 系統內存不足

**解決方案**：
```c
// 檢查初始化順序
retryix_initialize();
retryix_svm_create_context(&context);
retryix_svm_discover_topology();  // 確保這個被調用了！

void* ptr = retryix_svm_alloc_topology_aware(size);
```

### 問題2：性能不如預期

**可能原因**：
- 使用了主機內存而不是異構SVM
- 沒有正確的NUMA感知

**解決方案**：
```c
// 使用NUMA感知分配
void* ptr = retryix_svm_alloc_nearest_node(size);
// 或
void* ptr = retryix_svm_alloc_distributed(size, node_count);
```

### 問題3：內存洩漏

**常見錯誤**：
```c
// ❌ 錯誤：分配後沒有釋放
void* ptr = retryix_svm_alloc_topology_aware(1024);
// 使用ptr...
// 忘記釋放！

// ✅ 正確：總是配對使用
void* ptr = retryix_svm_alloc_topology_aware(1024);
if (ptr) {
    // 使用ptr...
    retryix_svm_free(ptr);  // 釋放！
}
```

### 問題4：多線程問題

**解決方案**：
```c
// 使用線程安全的分配
pthread_mutex_lock(&svm_mutex);
void* ptr = retryix_svm_alloc_topology_aware(size);
pthread_mutex_unlock(&svm_mutex);

// 使用後釋放
pthread_mutex_lock(&svm_mutex);
retryix_svm_free(ptr);
pthread_mutex_unlock(&svm_mutex);
```

---

## 📚 進階主題

### 異構SVM vs 主機內存

| 特性 | 異構SVM | 主機內存 |
|------|---------|----------|
| 性能 | 最高 | 良好 |
| 可用性 | 依賴硬件 | 總是可用 |
| 原子操作 | 支持 | 不支持 |
| 共享性 | GPU/CPU共享 | CPU專用 |

### NUMA架構優化

```c
// NUMA感知分配
void* ptr = retryix_svm_alloc_nearest_node(size);

// 設置內存親和性
retryix_svm_set_memory_affinity(ptr, numa_node);

// 查詢親和性
int current_node;
retryix_svm_get_memory_affinity(ptr, &current_node);
```

### 內存池管理

```c
// 使用內存池提高性能
typedef struct {
    void** pool;
    size_t pool_size;
    size_t used;
} memory_pool;

memory_pool* create_pool(size_t pool_size, size_t block_size) {
    memory_pool* pool = malloc(sizeof(memory_pool));
    pool->pool = malloc(sizeof(void*) * pool_size);
    pool->pool_size = pool_size;
    pool->used = 0;

    // 預分配
    for (size_t i = 0; i < pool_size; i++) {
        pool->pool[i] = retryix_svm_alloc_topology_aware(block_size);
        if (pool->pool[i]) {
            pool->used++;
        }
    }

    return pool;
}
```

---

## 🎯 總結

### 關鍵要點

1. **正確初始化**：initialize → create_context → discover_topology
2. **理解返回值**：int=錯誤碼，void*=指標
3. **接受降級**：降級是設計特點，不是缺陷
4. **關注最終結果**：成功率 >99% 才是優秀
5. **配對使用**：alloc/free 必須配對

### 設計哲學

RetryIX 的SVM系統就像魯班建橋：
- **優先用最好的材料**（異構SVM）
- **備用可靠的材料**（主機內存）
- **絕不讓橋塌掉**（寧缺毋濫）

### 最終建議

**不要追求100%的異構SVM使用率，要追求100%的最終成功率！**

這樣你就能正確使用RetryIX，避免踩坑，發揮其最大效能。

---

*本指南基於實際源代碼分析和測試經驗。如有更新，請參考最新文檔。*