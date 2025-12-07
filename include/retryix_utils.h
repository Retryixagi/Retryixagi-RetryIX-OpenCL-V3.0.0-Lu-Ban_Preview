#ifndef RETRYIX_UTILS_H
#define RETRYIX_UTILS_H

#include "retryix_core.h"
#include "retryix_device.h"
#include "retryix_svm.h"
#include "retryix_kernel.h"


// retryix_device_t 型別已在 retryix_device.h 定義，這裡不 forward declare。

// Forward declaration for retryix_platform_t
#ifndef RETRYIX_PLATFORM_T_DEFINED
typedef struct retryix_platform_t retryix_platform_t;
#define RETRYIX_PLATFORM_T_DEFINED
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ===== 系統資源查詢結構體 =====
typedef struct {
    char platforms[RETRYIX_MAX_JSON_OUTPUT];
    char devices[RETRYIX_MAX_JSON_OUTPUT];
    char svm_info[RETRYIX_MAX_JSON_OUTPUT];
    char kernels[RETRYIX_MAX_JSON_OUTPUT];
    char modules[RETRYIX_MAX_JSON_OUTPUT];
    char runtime_status[RETRYIX_MAX_JSON_OUTPUT];
} retryix_system_resources_t;

// ===== 性能評測結果 =====
typedef struct {
    double memory_bandwidth_gbps;
    double compute_gflops;
    double atomic_ops_per_second;
    double svm_latency_ns;
    int device_score;
    char details[1024];
} retryix_benchmark_result_t;

// ===== 診斷信息結構體 =====
typedef struct {
    int opencl_available;
    int platforms_found;
    int devices_found;
    int svm_support_level;
    int atomic_support;
    char driver_versions[512];
    char compatibility_issues[1024];
    char recommendations[1024];
} retryix_diagnostic_info_t;

// ===== 系統資源查詢 =====
/**
 * @brief 查詢所有系統資源並輸出 JSON
 * @param json_output JSON 輸出緩衝區
 * @param max_json_len 最大 JSON 長度
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_query_all_resources(
    char* json_output,
    size_t max_json_len
);

/**
 * @brief 查詢詳細系統資源
 * @param resources 系統資源結構體輸出
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_query_system_resources(
    retryix_system_resources_t* resources
);

/**
 * @brief 導出所有平台與設備信息到 JSON 檔案
 * @param output_path 輸出檔案路徑
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_export_all_devices_json(
    const char* output_path
);

// ===== 設備兼容性檢查 =====
/**
 * @brief 執行完整的設備兼容性檢查
 * @param diagnostic_info 診斷信息輸出
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_device_compatibility_check(
    retryix_diagnostic_info_t* diagnostic_info
);

/**
 * @brief 檢查特定設備的兼容性
 * @param device 目標設備
 * @param diagnostic_info 診斷信息輸出
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_check_device_compatibility(
    const retryix_device_t* device,
    retryix_diagnostic_info_t* diagnostic_info
);

/**
 * @brief 運行設備兼容性演示（類似您的 Python 腳本）
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
/* Demo utility functions are internal. Include "include/internal/retryix_demos.h"
   to access them, or define RETRYIX_DEMOS_PUBLIC to expose them from public headers
   (not recommended for release builds). */

#if defined(RETRYIX_DEMOS_PUBLIC) && RETRYIX_DEMOS_PUBLIC
#include "internal/retryix_demos.h"
#endif

// ===== JSON 工具函數 =====
/**
 * @brief 將設備信息轉換為 JSON 字符串
 * @param device 設備結構體
 * @param json_output JSON 輸出緩衝區
 * @param max_json_len 最大 JSON 長度
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_device_to_json(
    const retryix_device_t* device,
    char* json_output,
    size_t max_json_len
);

/**
 * @brief 將平台信息轉換為 JSON 字符串
 * @param platform 平台結構體
 * @param json_output JSON 輸出緩衝區
 * @param max_json_len 最大 JSON 長度
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_platform_to_json(
    const retryix_platform_t* platform,
    char* json_output,
    size_t max_json_len
);

/**
 * @brief 將診斷信息轉換為 JSON 字符串
 * @param diagnostic_info 診斷信息結構體
 * @param json_output JSON 輸出緩衝區
 * @param max_json_len 最大 JSON 長度
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_diagnostic_to_json(
    const retryix_diagnostic_info_t* diagnostic_info,
    char* json_output,
    size_t max_json_len
);

/**
 * @brief 將評測結果轉換為 JSON 字符串
 * @param benchmark_result 評測結果結構體
 * @param json_output JSON 輸出緩衝區
 * @param max_json_len 最大 JSON 長度
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_benchmark_to_json(
    const retryix_benchmark_result_t* benchmark_result,
    char* json_output,
    size_t max_json_len
);

// ===== 檔案 I/O 工具 =====
/**
 * @brief 載入文字檔案內容
 * @param filename 檔案路徑
 * @param content 內容輸出緩衝區
 * @param max_content_len 最大內容長度
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_load_text_file(
    const char* filename,
    char* content,
    size_t max_content_len
);

/**
 * @brief 保存文字內容到檔案
 * @param filename 檔案路徑
 * @param content 要保存的內容
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_save_text_file(
    const char* filename,
    const char* content
);

/**
 * @brief 追加文字內容到檔案
 * @param filename 檔案路徑
 * @param content 要追加的內容
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_append_text_file(
    const char* filename,
    const char* content
);

// ===== 字符串工具 =====
/**
 * @brief 安全的字符串複製
 * @param dest 目標緩衝區
 * @param src 源字符串
 * @param dest_size 目標緩衝區大小
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_safe_strcpy(
    char* dest,
    const char* src,
    size_t dest_size
);

/**
 * @brief 安全的字符串格式化
 * @param buffer 輸出緩衝區
 * @param buffer_size 緩衝區大小
 * @param format 格式字符串
 * @param ... 格式參數
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_safe_sprintf(
    char* buffer,
    size_t buffer_size,
    const char* format,
    ...
);

/**
 * @brief 去除字符串兩端空白字符
 * @param str 要處理的字符串
 * @return 處理後的字符串指針
 */
RETRYIX_API char* RETRYIX_CALL retryix_trim_string(char* str);

// ===== 時間與計時工具 =====
/**
 * @brief 獲取高精度時間戳（微秒）
 * @return 時間戳
 */
RETRYIX_API uint64_t RETRYIX_CALL retryix_get_timestamp_us(void);

/**
 * @brief 獲取高精度時間戳（納秒）
 * @return 時間戳
 */
RETRYIX_API uint64_t RETRYIX_CALL retryix_get_timestamp_ns(void);

/**
 * @brief 計算兩個時間戳之間的差值（微秒）
 * @param start_time 開始時間
 * @param end_time 結束時間
 * @return 時間差值
 */
RETRYIX_API uint64_t RETRYIX_CALL retryix_time_diff_us(
    uint64_t start_time,
    uint64_t end_time
);

// ===== 記憶體工具 =====
/**
 * @brief 安全的記憶體分配
 * @param size 分配大小
 * @param zero_memory 是否將記憶體清零
 * @return 分配的記憶體指針，失敗時返回 NULL
 */
RETRYIX_API void* RETRYIX_CALL retryix_safe_malloc(
    size_t size,
    int zero_memory
);

/**
 * @brief 安全的記憶體重新分配
 * @param ptr 原記憶體指針
 * @param new_size 新大小
 * @return 重新分配的記憶體指針，失敗時返回 NULL
 */
RETRYIX_API void* RETRYIX_CALL retryix_safe_realloc(
    void* ptr,
    size_t new_size
);

/**
 * @brief 安全的記憶體釋放
 * @param ptr 要釋放的記憶體指針
 */
RETRYIX_API void RETRYIX_CALL retryix_safe_free(void* ptr);

// ===== 日誌記錄工具 =====
typedef enum {
    RETRYIX_LOG_LEVEL_DEBUG = 0,
    RETRYIX_LOG_LEVEL_INFO = 1,
    RETRYIX_LOG_LEVEL_WARNING = 2,
    RETRYIX_LOG_LEVEL_ERROR = 3,
    RETRYIX_LOG_LEVEL_CRITICAL = 4
} retryix_log_level_t;

/**
 * @brief 設置日誌記錄等級
 * @param level 日誌等級
 */
RETRYIX_API void RETRYIX_CALL retryix_set_log_level(retryix_log_level_t level);

/**
 * @brief 記錄日誌信息
 * @param level 日誌等級
 * @param format 格式字符串
 * @param ... 格式參數
 */
RETRYIX_API void RETRYIX_CALL retryix_log(
    retryix_log_level_t level,
    const char* format,
    ...
);

/**
 * @brief 設置日誌輸出檔案
 * @param filename 日誌檔案路徑（NULL 表示輸出到控制台）
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_set_log_file(
    const char* filename
);

// ===== 便利宏定義 =====
#define RETRYIX_LOG_DEBUG(fmt, ...) \
    retryix_log(RETRYIX_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

#define RETRYIX_LOG_INFO(fmt, ...) \
    retryix_log(RETRYIX_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)

#define RETRYIX_LOG_WARNING(fmt, ...) \
    retryix_log(RETRYIX_LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)

#define RETRYIX_LOG_ERROR(fmt, ...) \
    retryix_log(RETRYIX_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define RETRYIX_LOG_CRITICAL(fmt, ...) \
    retryix_log(RETRYIX_LOG_LEVEL_CRITICAL, fmt, ##__VA_ARGS__)

// 檢查指針是否為空的宏
#define RETRYIX_CHECK_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            RETRYIX_LOG_ERROR("Null pointer detected: %s at %s:%d", #ptr, __FILE__, __LINE__); \
            return RETRYIX_ERROR_NULL_PTR; \
        } \
    } while(0)

// 檢查 RetryIX 函數返回值的宏
#define RETRYIX_CHECK_RESULT(call) \
    do { \
        retryix_result_t _result = (call); \
        if (_result != RETRYIX_SUCCESS) { \
            RETRYIX_LOG_ERROR("Function call failed: %s returned %d at %s:%d", #call, _result, __FILE__, __LINE__); \
            return _result; \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif // RETRYIX_UTILS_H