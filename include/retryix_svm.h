// retryix_svm.h - RetryIX SVM Public API v3.0.0
#ifndef RETRYIX_SVM_H
#define RETRYIX_SVM_H

// === Core Includes ===
#include "retryix_opencl_compat.h"
#include "retryix_export.h"
#include "retryix_svm_types.h"
#include "retryix_svm_context.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// === OpenCL 類型由 retryix_opencl_compat.h 統一定義 ===
// 不再重複定義

// === Extern C Linkage (開啟一次) ===
#ifdef __cplusplus
extern "C" {
#endif

// === 版本信息 ===
#define RETRYIX_SVM_VERSION_MAJOR 3
#define RETRYIX_SVM_VERSION_MINOR 0
#define RETRYIX_SVM_VERSION_PATCH 0

// === 前向宣告 (單次宣告) ===
typedef struct retryix_svm_context retryix_svm_context_t;

// === 錯誤碼定義 ===
typedef enum {
    RETRYIX_SVM_SUCCESS = 0,
    RETRYIX_SVM_ERROR_INVALID_PARAM = -1,
    RETRYIX_SVM_ERROR_OUT_OF_MEMORY = -2,
    RETRYIX_SVM_ERROR_DEVICE_NOT_FOUND = -3,
    RETRYIX_SVM_ERROR_NOT_SUPPORTED = -4,
    RETRYIX_SVM_ERROR_CONTEXT_INVALID = -5,
    RETRYIX_SVM_ERROR_ALLOCATION_FAILED = -6,
    RETRYIX_SVM_ERROR_MAPPING_FAILED = -7,
    RETRYIX_SVM_ERROR_NOT_MAPPED = -8,
    RETRYIX_SVM_ERROR_ALIGNMENT = -9,
    RETRYIX_SVM_ERROR_SIZE_EXCEEDED = -10,
    RETRYIX_SVM_ERROR_THREAD_SAFETY = -11,
    RETRYIX_SVM_ERROR_INTERNAL = -100
} retryix_svm_result_t;

// === v3.0.0: 128/256-bit 原子操作型別定義 ===
// 拆分 macro：區分「有 u128_t 型別」與「有原生硬體支援」
#if defined(__GNUC__) || defined(__clang__)
    // GCC/Clang: 原生 unsigned __int128 型別 + 編譯器支援 __atomic built-ins
    typedef unsigned __int128 u128_t;
    #define RETRYIX_HAS_INT128_TYPE 1
    #define RETRYIX_HAS_INT128_NATIVE 1  // 支援 cmpxchg16b 等原生指令
#elif defined(_MSC_VER) && defined(_M_X64)
    // MSVC: 使用 struct 模擬 128-bit + 支援 _InterlockedCompareExchange128
    typedef struct {
        uint64_t lo;
        uint64_t hi;
    } u128_t;
    #define RETRYIX_HAS_INT128_TYPE 1
    #define RETRYIX_HAS_INT128_NATIVE 0  // 使用模擬/intrinsic wrapper
#else
    // 不支援平台
    #define RETRYIX_HAS_INT128_TYPE 0
    #define RETRYIX_HAS_INT128_NATIVE 0
#endif

// 對齊要求宏定義 (v3.0.0)
#define RETRYIX_ALIGN_128 16  // 128-bit atomic 需要 16-byte 對齊
#define RETRYIX_ALIGN_256 32  // 256-bit pair 需要 32-byte 對齊

// 256-bit pair (雙 128-bit)
#if RETRYIX_HAS_INT128_TYPE
typedef struct {
    u128_t lo;
    u128_t hi;
} u256_pair_t;
#endif

// === v3.0.0: 原子能力 Bitmask 定義 ===
#define RETRYIX_ATOMIC_CAP_NONE            0u
#define RETRYIX_ATOMIC_CAP_32BIT           (1u << 0)  // 支援 32-bit 原子操作
#define RETRYIX_ATOMIC_CAP_64BIT           (1u << 1)  // 支援 64-bit 原子操作
#define RETRYIX_ATOMIC_CAP_128_EMULATED    (1u << 2)  // 支援 128-bit (軟體模擬)
#define RETRYIX_ATOMIC_CAP_128_NATIVE      (1u << 3)  // 支援 128-bit (硬體原生)
#define RETRYIX_ATOMIC_CAP_256_PAIR        (1u << 4)  // 支援 256-bit pair CAS

// 原子統計結構（v3.0.0 擴展）
typedef struct {
    uint64_t atomic_ops_total;      // 原子操作總次數
    uint64_t atomic_ops_failed;     // 原子操作失敗次數
    uint64_t atomic_ops_success;    // 原子操作成功次數
    uint64_t atomic_ops_conflicts;  // 原子操作衝突次數
    uint64_t atomic_128bit_ops;     // 128-bit 原子操作次數 (v3.0.0)
    uint64_t atomic_256bit_ops;     // 256-bit 原子操作次數 (v3.0.0)
    uint64_t atomic_fast_path;      // 硬體快速路徑次數 (v3.0.0)
    uint64_t atomic_slow_path;      // 軟體慢速路徑次數 (v3.0.0)
} retryix_svm_atomic_stats_t;

// === 原子能力查詢 API (v3.0.0 改善版) ===
/**
 * 查詢平台原子能力 (返回 bitmask)
 * @param ctx SVM 上下文
 * @return 原子能力 bitmask (RETRYIX_ATOMIC_CAP_*)，失敗返回 0
 * @note 使用 bitmask 檢查：if (caps & RETRYIX_ATOMIC_CAP_128_NATIVE) { ... }
 */
RETRYIX_API uint32_t RETRYIX_CALL retryix_svm_atomic_capabilities(const retryix_svm_context_t* ctx);

/**
 * 查詢是否支援原子操作 (舊版相容 API)
 * @deprecated 建議使用 retryix_svm_atomic_capabilities() 取得詳細能力
 */
RETRYIX_API int RETRYIX_CALL retryix_svm_has_atomic_support(const retryix_svm_context_t* context);

// 原子統計 API
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_reset_atomic_stats(retryix_svm_context_t* context);
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_get_atomic_stats(retryix_svm_context_t* context, retryix_svm_atomic_stats_t* out);

// === 原子操作 API (32/64-bit) ===
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_sub_int32(retryix_svm_context_t* context, volatile int32_t* ptr, int32_t value, int32_t* old_value);
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_and_int32(retryix_svm_context_t* context, volatile int32_t* ptr, int32_t value, int32_t* old_value);
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_or_int32(retryix_svm_context_t* context, volatile int32_t* ptr, int32_t value, int32_t* old_value);
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_xor_int32(retryix_svm_context_t* context, volatile int32_t* ptr, int32_t value, int32_t* old_value);
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_exchange_int32(retryix_svm_context_t* context, volatile int32_t* ptr, int32_t value, int32_t* old_value);
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_min_int32(retryix_svm_context_t* context, volatile int32_t* ptr, int32_t value, int32_t* old_value);

// === v3.0.0: 128-bit 原子操作 API ===
#if RETRYIX_HAS_INT128_TYPE
/**
 * 128-bit Fetch-Add 原子操作
 * @param ctx SVM 上下文
 * @param ptr 128-bit 指標 (必須 16-byte 對齊)
 * @param value 要加上的值
 * @param old_value 輸出：操作前的舊值
 * @return RETRYIX_SVM_SUCCESS 成功，RETRYIX_SVM_ERROR_ALIGNMENT 未對齊
 * @note Memory order: Sequential consistency (seq-cst)
 * @note 若硬體支援將使用 cmpxchg16b (fast path)，否則 spinlock (slow path)
 */
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_fetch_add_i128(
    retryix_svm_context_t* ctx,
    volatile u128_t* ptr,
    u128_t value,
    u128_t* old_value
);

/**
 * 128-bit Compare-Exchange 原子操作
 * @param ctx SVM 上下文
 * @param ptr 128-bit 指標 (必須 16-byte 對齊)
 * @param expected 輸入/輸出：期望值/實際讀取值
 * @param desired 若匹配則寫入此值
 * @return RETRYIX_SVM_SUCCESS 成功交換，RETRYIX_SVM_ERROR_INTERNAL 交換失敗
 * @note Memory order: Sequential consistency (seq-cst)
 */
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_compare_exchange_i128(
    retryix_svm_context_t* ctx,
    volatile u128_t* ptr,
    u128_t* expected,
    u128_t desired
);

/**
 * 128-bit Exchange 原子操作
 * @param ctx SVM 上下文
 * @param ptr 128-bit 指標 (必須 16-byte 對齊)
 * @param value 要寫入的新值
 * @param old_value 輸出：操作前的舊值
 * @return RETRYIX_SVM_SUCCESS 成功
 * @note Memory order: Sequential consistency (seq-cst)
 */
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_exchange_i128(
    retryix_svm_context_t* ctx,
    volatile u128_t* ptr,
    u128_t value,
    u128_t* old_value
);

/**
 * 128-bit Load 原子操作
 * @param ctx SVM 上下文
 * @param ptr 128-bit 指標 (必須 16-byte 對齊)
 * @param out_value 輸出：讀取的值
 * @return RETRYIX_SVM_SUCCESS 成功
 * @note Memory order: Sequential consistency (seq-cst)
 */
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_load_i128(
    retryix_svm_context_t* ctx,
    volatile u128_t* ptr,
    u128_t* out_value
);

/**
 * 128-bit Store 原子操作
 * @param ctx SVM 上下文
 * @param ptr 128-bit 指標 (必須 16-byte 對齊)
 * @param value 要寫入的值
 * @return RETRYIX_SVM_SUCCESS 成功
 * @note Memory order: Sequential consistency (seq-cst)
 */
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_store_i128(
    retryix_svm_context_t* ctx,
    volatile u128_t* ptr,
    u128_t value
);
#endif // RETRYIX_HAS_INT128_TYPE

// === v3.0.0: 256-bit 原子操作 API (Pair CAS) ===
#if RETRYIX_HAS_INT128_TYPE
/**
 * 256-bit Pair Compare-Exchange 原子操作
 * @param ctx SVM 上下文
 * @param ptr_lo 低 128-bit 指標 (必須 16-byte 對齊)
 * @param ptr_hi 高 128-bit 指標 (必須 16-byte 對齊)
 * @param expected 輸入/輸出：期望的 256-bit pair / 實際讀取值
 * @param desired 若匹配則寫入此 pair
 * @return RETRYIX_SVM_SUCCESS 成功，RETRYIX_SVM_ERROR_INTERNAL 交換失敗
 * @note 使用雙重 spinlock，按地址排序避免死鎖
 * @note 無 ABA 問題（雙重鎖定保證可見性）
 * @note Memory order: Sequential consistency (seq-cst)
 */
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_atomic_compare_exchange_pair_256(
    retryix_svm_context_t* ctx,
    volatile u128_t* ptr_lo,
    volatile u128_t* ptr_hi,
    u256_pair_t* expected,
    u256_pair_t desired
);
#endif // RETRYIX_HAS_INT128_TYPE


// 型別定義已移至 retryix_svm_types.h (already included above)

// === 分配信息 ===
typedef struct {
    void* ptr;                         // 分配的指針
    size_t size;                       // 分配大小
    size_t actual_size;                // 實際分配大小（含對齊）
    retryix_svm_flags_t flags;         // 分配標誌
    retryix_svm_level_t level;         // 使用的SVM等級
    bool is_mapped;                    // 是否已映射
    uint64_t allocation_id;            // 分配唯一ID
} retryix_svm_alloc_info_t;

// === 回調函數類型 ===
typedef void (*retryix_svm_log_callback_t)(int level, const char* message, void* user_data);
typedef void (*retryix_svm_error_callback_t)(retryix_svm_result_t error, const char* details, void* user_data);

// =========================================================================
// === 核心API ===
// =========================================================================
RETRYIX_API void RETRYIX_CALL retryix_svm_get_default_config(retryix_svm_config_t* config);
/**
 * 獲取默認配置
 */
/**
 * 查詢設備SVM能力
 */
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_query_device_capabilities(
    cl_device_id device,
    retryix_svm_device_info_t* info
);

/**
 * 創建SVM上下文
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_create_context(
    cl_context cl_context,
    cl_device_id device,
    const retryix_svm_config_t* config,
    retryix_svm_context_t** out_context
);

/**
 * 銷毀SVM上下文
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_destroy_context(
    retryix_svm_context_t* context
);

// =========================================================================
// === 內存管理API ===
// =========================================================================

/**
 * 分配SVM內存
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_alloc(
    retryix_svm_context_t* context,
    size_t size,
    retryix_svm_flags_t flags,
    void** out_ptr
);

/**
 * 對齊分配SVM內存
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_alloc_aligned(
    retryix_svm_context_t* context,
    size_t size,
    size_t alignment,
    retryix_svm_flags_t flags,
    void** out_ptr
);

/**
 * 批量分配SVM內存
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_alloc_batch(
    retryix_svm_context_t* context,
    size_t count,
    const size_t* sizes,
    const retryix_svm_flags_t* flags,
    void** out_ptrs
);

/**
 * 釋放SVM內存
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_free(
    retryix_svm_context_t* context,
    void* ptr
);

/**
 * 批量釋放SVM內存
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_free_batch(
    retryix_svm_context_t* context,
    size_t count,
    void** ptrs
);

// =========================================================================
// === 內存映射API ===
// =========================================================================

/**
 * 映射SVM內存
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_map(
    retryix_svm_context_t* context,
    void* ptr,
    cl_command_queue queue,
    cl_map_flags map_flags
);

/**
 * 解除映射SVM內存
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_unmap(
    retryix_svm_context_t* context,
    void* ptr,
    cl_command_queue queue
);

/**
 * 同步SVM內存
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_sync(
    retryix_svm_context_t* context,
    void* ptr,
    size_t size,
    cl_command_queue queue
);

// =========================================================================
// === 信息查詢API ===
// =========================================================================

/**
 * 獲取分配信息
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_get_allocation_info(
    retryix_svm_context_t* context,
    void* ptr,
    retryix_svm_alloc_info_t* info
);

/**
 * 獲取統計信息
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_get_statistics(
    retryix_svm_context_t* context,
    retryix_svm_stats_t* stats
);

/**
 * 重置統計信息
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_reset_statistics(
    retryix_svm_context_t* context
);

// =========================================================================
// === 實用工具API ===
// =========================================================================

/**
 * 檢查指針是否為有效SVM指針
 */
    RETRYIX_API bool RETRYIX_CALL retryix_svm_is_valid_ptr(
    retryix_svm_context_t* context,
    void* ptr
);

/**
 * 獲取錯誤描述
 */
    RETRYIX_API const char* RETRYIX_CALL retryix_svm_get_error_string(
    retryix_svm_result_t result
);

/**
 * 設置日誌回調
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_set_log_callback(retryix_svm_log_callback_t callback, void *user_data);

/**
 * 設置錯誤回調
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_set_error_callback(
    retryix_svm_error_callback_t callback,
    void* user_data
);

// =========================================================================
// === 內存池API ===
// =========================================================================

/**
 * 預分配內存池
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_pool_reserve(
    retryix_svm_context_t* context,
    size_t size
);

/**
 * 收縮內存池
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_pool_shrink(
    retryix_svm_context_t* context
);

/**
 * 清空內存池
 */
    RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_pool_clear(
    retryix_svm_context_t* context
);

// =========================================================================
// === v3.0.0: u128 Helper APIs (MSVC Emulation Support) ===
// =========================================================================
#if RETRYIX_HAS_INT128_TYPE && !RETRYIX_HAS_INT128_NATIVE
/**
 * u128 helper: 從兩個 uint64_t 建立 u128_t
 * @note 僅在 MSVC (struct-based u128) 編譯時可用
 */
static inline u128_t retryix_u128_from_u64pair(uint64_t lo, uint64_t hi) {
    u128_t result;
    result.lo = lo;
    result.hi = hi;
    return result;
}

/**
 * u128 helper: 拆解 u128_t 為兩個 uint64_t
 */
static inline void retryix_u128_to_u64pair(u128_t val, uint64_t* out_lo, uint64_t* out_hi) {
    *out_lo = val.lo;
    *out_hi = val.hi;
}

/**
 * u128 helper: 相等比較
 */
static inline bool retryix_u128_eq(u128_t a, u128_t b) {
    return (a.lo == b.lo) && (a.hi == b.hi);
}

/**
 * u128 helper: 加法 (不考慮溢出)
 */
static inline u128_t retryix_u128_add(u128_t a, u128_t b) {
    u128_t result;
    result.lo = a.lo + b.lo;
    result.hi = a.hi + b.hi;
    if (result.lo < a.lo) result.hi++; // Carry
    return result;
}
#endif // RETRYIX_HAS_INT128_TYPE && !RETRYIX_HAS_INT128_NATIVE

// =========================================================================
// === 便利宏定義 ===
// =========================================================================

#define RETRYIX_SVM_CHECK(expr) \
    do { \
        retryix_svm_result_t _result = (expr); \
        if (_result != RETRYIX_SVM_SUCCESS) { \
            return _result; \
        } \
    } while (0)

#define RETRYIX_SVM_DEFAULT_ALIGNMENT 64
#define RETRYIX_SVM_DEFAULT_POOL_SIZE (16 * 1024 * 1024)  // 16MB

// =========================================================================
// === 輔助 API ===
// =========================================================================
/**
 * 向量加法 API (測試用輔助函式)
 */
RETRYIX_API int RETRYIX_CALL retryix_vector_add(float* a, float* b, float* result, int n);

// =========================================================================
// === Extern C 結束 ===
// =========================================================================
#ifdef __cplusplus
}
#endif

#endif // RETRYIX_SVM_H