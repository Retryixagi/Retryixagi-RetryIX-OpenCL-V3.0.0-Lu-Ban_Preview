
// ===== include guard =====
#ifndef RETRYIX_H
#define RETRYIX_H

#include "retryix_export.h"

// OpenCL 兼容層 - 必須最先包含
#include "retryix_opencl_compat.h"

// ===== 型別定義必須在 API 宣告前 =====
#include "retryix_core.h"        // 核心系統定義,確保 retryix_result_t 已定義 (包含 OpenCL 類型模擬)
#include "retryix_device.h"      // 設備管理
#include "retryix_svm.h"         // SVM 記憶體管理
#include "retryix_kernel.h"      // Kernel 管理
#include "retryix_utils.h"       // 工具函數
#include "retryix_benchmark.h"   // Benchmark 結構定義
#include "retryix_host.h"        // Host 通信

// ===== Bridge 模組 (GPU 廠商支持) =====
#include "retryix_bridge_cuda.h"        // NVIDIA CUDA
#include "retryix_bridge_rocm.h"        // AMD ROCm
#include "retryix_bridge_intel_l0.h"    // Intel Level-Zero
#include "retryix_bridge_opencl_intel.h" // Intel OpenCL
#include "retryix_bridge_cpu_intel.h"   // Intel CPU

// ===== 高級功能模組 =====
#include "retryix_zerocopy.h"    // 零拷貝網絡 (RDMA/DMA/GPU Direct)
#include "retryix_comm.h"        // 通信模組
#include "retryix_host_comm.h"   // Host 通信
#include "retryix_atomic.h"      // 原子操作

// ===== 平台專屬擴充 =====
#ifdef _WIN32
    #include "retryix_platform_win.h"
#elif defined(__APPLE__)
    #include "retryix_platform_mac.h"
#elif defined(__linux__)
    #include "retryix_platform_linux.h"
#endif
// ===== 全域系統狀態 =====
// Ensure C linkage for C++ compilers
#ifdef __cplusplus
extern "C" {
#endif


// ===== 全域系統狀態 =====
typedef struct {
    int is_initialized;
    int device_count;
    int platform_count;
    retryix_device_t best_device;
    retryix_svm_context_t* svm_context;
    retryix_kernel_manager_t kernel_manager;
    retryix_diagnostic_info_t last_diagnostic;
} retryix_system_state_t;

// ---- CLI 相關 API 補齊 ----

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_platforms(
    retryix_platform_t* platforms,
    int max_platforms,
    int* platform_count
);
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_devices_for_platform(
    cl_platform_id platform_id,
    retryix_device_t* devices,
    int max_devices,
    int* device_count
);
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_all_devices(
    retryix_device_t* devices,
    int max_devices,
    int* device_count
);
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_select_best_device(
    const retryix_device_t* devices,
    int device_count,
    retryix_device_t* best_device
);
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_filter_devices_by_type(
    const retryix_device_t* devices,
    int device_count,
    cl_device_type device_type,
    retryix_device_t* filtered_devices,
    int max_filtered,
    int* filtered_count
);
RETRYIX_API int RETRYIX_CALL retryix_device_supports_capability(
    const retryix_device_t* device,
    retryix_device_capabilities_t capability
);
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_check_svm_support(
    const retryix_device_t* device,
    int* supports_coarse,
    int* supports_fine,
    int* supports_atomic
);
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_check_atomic_support(
    const retryix_device_t* device,
    int* supports_int32,
    int* supports_int64
);
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_export_devices_json(
    const retryix_device_t* devices,
    int device_count,
    char* json_output,
    size_t max_json_len
);
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_save_devices_to_file(
    const retryix_device_t* devices,
    int device_count,
    const char* filename
);
RETRYIX_API void RETRYIX_CALL retryix_shutdown(void);

/**
 * @brief 獲取當前系統狀態
 * @return 系統狀態結構體指針
/* Demos (including retryix_run_python_equivalent_demo) are internal and have
// ===== 一鍵式高級 API =====
/**
 * @brief 一鍵初始化整個 RetryIX 系統
 * 
 * 這個函數會自動：
 * 1. 初始化核心系統
 * 2. 發現所有可用設備
 * 3. 選擇最佳設備
 * 4. 創建 SVM 上下文（如果支援）
 * 5. 初始化 Kernel 管理器
 * 
 * @param prefer_gpu 是否優先選擇 GPU 設備
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_auto_initialize(int prefer_gpu);

/**
 * @brief 一鍵執行完整的兼容性檢查和性能評測
 * 
 * 這個函數會自動：
 * 1. 運行設備兼容性檢查
 * 2. 執行 SVM 演示測試
 * 3. 執行原子操作測試
 * 4. 運行性能評測
 * 5. 生成完整的 JSON 報告
 * 
 * @param json_report JSON 報告輸出緩衝區
 * @param max_report_len 最大報告長度
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_full_system_check(
    char* json_report,
    size_t max_report_len
);

/**
 * @brief Check if SVM subsystem was initialized
 */
RETRYIX_API int RETRYIX_CALL retryix_is_svm_initialized(void);

/**
 * @brief Get current SVM level (retryix_svm_level_t)
 */
RETRYIX_API int RETRYIX_CALL retryix_get_svm_level(void);

/**
/* Demos (including retryix_run_python_equivalent_demo) are internal and have
    been moved to include/internal/retryix_demos.h. To enable their prototypes
    in public headers, define RETRYIX_DEMOS_PUBLIC before including headers
    (not recommended for release builds). */

#if defined(RETRYIX_DEMOS_PUBLIC) && RETRYIX_DEMOS_PUBLIC
#include "internal/retryix_demos.h"
#endif

// ===== 便利包裝函數 =====
/**
 * @brief 快速獲取最佳設備信息
 * @param device 設備信息輸出
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_get_best_device(
    retryix_device_t* device
);

/**
 * @brief 快速分配 SVM 記憶體
 * @param size 分配大小
 * @param allocation 分配描述符輸出
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_quick_svm_alloc(
    size_t size,
    retryix_svm_allocation_t* allocation
);

/**
 * @brief 快速釋放 SVM 記憶體
 * @param allocation 分配描述符
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_quick_svm_free(
    retryix_svm_allocation_t* allocation
);

/**
 * @brief 快速編譯並執行 Kernel
 * @param kernel_name Kernel 名稱
 * @param source_code 源代碼
 * @param global_work_size 全域工作大小
 * @param local_work_size 本地工作大小
 * @param args 參數數組
 * @param arg_count 參數數量
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_quick_kernel_run(
    const char* kernel_name,
    const char* source_code,
    size_t global_work_size,
    size_t local_work_size,
    retryix_kernel_arg_t* args,
    int arg_count
);

/**
 * @brief Backward compatibility: retryix_query_all_resources
 * 
 * This is the C version of the main function called in your Python script
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_query_all_resources(
    char* json_out,
    size_t max_len
);

/**
 * @brief Backward compatibility: retryix_kernel_atomic_add_demo
 * 
 * This is the direct equivalent function of your atomic operation demonstration
 */
// 請使用 retryix_kernel.h 的正確型別宣告

// ===== 預定義的 Kernel 源代碼 =====
extern const char* RETRYIX_KERNEL_ATOMIC_ADD;
extern const char* RETRYIX_KERNEL_MEMORY_TEST;
extern const char* RETRYIX_KERNEL_FLOAT_PERF;
extern const char* RETRYIX_KERNEL_SVM_TEST;

// ===== 系統信息宏 =====
#define RETRYIX_LIBRARY_NAME        "RetryIX"
#define RETRYIX_LIBRARY_DESCRIPTION "Universal OpenCL Device Compatibility & SVM Management Library"
#define RETRYIX_COPYRIGHT           "Copyright (c) 2024 RetryIX Team"
#define RETRYIX_LICENSE             "MIT License"

// ===== 編譯時配置檢查 =====
#ifdef RETRYIX_NO_OPENCL
    #pragma message("RetryIX compiled without OpenCL support - limited functionality")
#endif

#ifdef RETRYIX_DEBUG
    #define RETRYIX_DEBUG_ENABLED 1
    RETRYIX_API void RETRYIX_CALL retryix_debug_print_system_info(void);
#else
    #define RETRYIX_DEBUG_ENABLED 0
#endif

// ===== 特性檢測宏 =====
/**
 * @brief Check if SVM is supported
 */
#define RETRYIX_HAS_SVM() \
    (retryix_get_system_state()->svm_context && retryix_get_system_state()->svm_context->is_initialized)

/**
 * @brief Check if atomic operations are supported
 */
#define RETRYIX_HAS_ATOMICS() \
    (retryix_get_system_state()->best_device.capabilities & RETRYIX_CAP_ATOMIC_INT32)

/**
 * @brief 檢查是否支援 OpenGL 共享
 */
#define RETRYIX_HAS_GL_SHARING() \
    (retryix_get_system_state()->best_device.capabilities & RETRYIX_CAP_GL_SHARING)

// ===== 快速狀態查詢宏 =====
#define RETRYIX_IS_INITIALIZED() \
    (retryix_get_system_state()->is_initialized)

#define RETRYIX_GET_DEVICE_COUNT() \
    (retryix_get_system_state()->device_count)

#define RETRYIX_GET_PLATFORM_COUNT() \
    (retryix_get_system_state()->platform_count)

#define RETRYIX_GET_BEST_DEVICE_NAME() \
    (retryix_get_system_state()->best_device.name)

// ===== 錯誤處理助手宏 =====
#define RETRYIX_ENSURE_INITIALIZED() \
    do { \
        if (!RETRYIX_IS_INITIALIZED()) { \
            RETRYIX_LOG_ERROR("RetryIX not initialized - call retryix_initialize() first"); \
            return RETRYIX_ERROR_NULL_PTR; \
        } \
    } while(0)

#define RETRYIX_SAFE_CALL(func_call) \
    do { \
        retryix_result_t _result = (func_call); \
        if (_result != RETRYIX_SUCCESS) { \
            RETRYIX_LOG_ERROR("RetryIX call failed: %s", retryix_get_error_string(_result)); \
            return _result; \
        } \
    } while(0)

// ===== 使用範例文檔 =====
/**
 * @example basic_usage.c
 * 基本使用範例：
 * @code{.c}
 * #include "retryix.h"
 * #include <stdio.h>
 * 
 * int main() {
 *     // 自動初始化，優先使用 GPU
 *     if (retryix_auto_initialize(1) != RETRYIX_SUCCESS) {
 *         printf("Failed to initialize RetryIX\n");
 *         return -1;
 *     }
 *     
 *     printf("RetryIX initialized successfully!\n");
 *     printf("Best device: %s\n", RETRYIX_GET_BEST_DEVICE_NAME());
 *     printf("Device count: %d\n", RETRYIX_GET_DEVICE_COUNT());
 *     printf("SVM support: %s\n", RETRYIX_HAS_SVM() ? "Yes" : "No");
 *     
 *     // 運行兼容性檢查
 *     retryix_run_python_equivalent_demo();
 *     
 *     // 查詢系統資源
 *     char json_output[8192];
 *     retryix_query_all_resources(json_output, sizeof(json_output));
 *     printf("System resources:\n%s\n", json_output);
 *     
 *     // 清理
 *     retryix_cleanup();
 *     return 0;
 * }
 * @endcode
 */

/**
 * @example svm_usage.c
 * SVM 記憶體使用範例：
 * @code{.c}
 * #include "retryix.h"
 * 
 * int main() {
 *     retryix_auto_initialize(1);
 *     
 *     if (RETRYIX_HAS_SVM()) {
 *         retryix_svm_allocation_t allocation;
 *         
 *         // 分配 1MB SVM 記憶體
 *         if (retryix_quick_svm_alloc(1024*1024, &allocation) == RETRYIX_SUCCESS) {
 *             printf("SVM memory allocated: %p\n", allocation.ptr);
 *             
 *             // 使用記憶體...
 *             
 *             // 釋放記憶體
 *             retryix_quick_svm_free(&allocation);
 *         }
 *     }
 *     
 *     retryix_cleanup();
 *     return 0;
 * }
 * @endcode
 */

#ifdef __cplusplus
}
#endif

#endif // RETRYIX_H