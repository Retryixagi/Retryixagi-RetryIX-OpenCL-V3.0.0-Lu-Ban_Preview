#ifndef RETRYIX_KERNEL_H
#define RETRYIX_KERNEL_H

#include "retryix_opencl_compat.h"
#include "retryix_core.h"
#include "retryix_device.h"
#include "retryix_svm.h"

#ifdef __cplusplus
extern "C" {
#endif

// 避免與 retryix_compute_backend.h 中的類型衝突
#ifndef RETRYIX_KERNEL_TYPE_DEFINED
#define RETRYIX_KERNEL_TYPE_DEFINED

// ===== Kernel 相關常量 =====
#define RETRYIX_MAX_KERNEL_NAME_LEN     128
#define RETRYIX_MAX_KERNEL_SOURCE_LEN   65536
#define RETRYIX_MAX_BUILD_LOG_LEN       8192
#define RETRYIX_MAX_KERNEL_ARGS         32
#define RETRYIX_MAX_KERNELS             256

// ===== Kernel 參數類型 =====
typedef enum {
    RETRYIX_ARG_TYPE_BUFFER,
    RETRYIX_ARG_TYPE_IMAGE,
    RETRYIX_ARG_TYPE_SVM_POINTER,
    RETRYIX_ARG_TYPE_LOCAL_MEMORY,
    RETRYIX_ARG_TYPE_SCALAR_INT32,
    RETRYIX_ARG_TYPE_SCALAR_INT64,
    RETRYIX_ARG_TYPE_SCALAR_FLOAT,
    RETRYIX_ARG_TYPE_SCALAR_DOUBLE
} retryix_kernel_arg_type_t;

// ===== Kernel 參數結構體 =====
typedef struct {
    retryix_kernel_arg_type_t type;
    size_t size;
    union {
        void* buffer_ptr;
        void* image_ptr;
        void* svm_ptr;
        size_t local_mem_size;
        int32_t scalar_int32;
        int64_t scalar_int64;
        float scalar_float;
        double scalar_double;
    } value;
} retryix_kernel_arg_t;

// ===== Kernel 執行配置 =====
typedef struct {
    size_t global_work_size[3];
    size_t local_work_size[3];
    size_t global_work_offset[3];
    int work_dimensions;
} retryix_kernel_config_t;

// ===== Kernel 程序結構體 =====
typedef struct {
    char name[RETRYIX_MAX_KERNEL_NAME_LEN];
    char source_code[RETRYIX_MAX_KERNEL_SOURCE_LEN];
    char build_options[512];
    char build_log[RETRYIX_MAX_BUILD_LOG_LEN];
    
    void* cl_program;
    void* cl_kernel;
    void* cl_context;
    void* cl_device;
    
    int is_compiled;
    int is_built;
    int arg_count;
    retryix_kernel_arg_t args[RETRYIX_MAX_KERNEL_ARGS];
    
    // 性能統計
    double last_execution_time;
    double total_execution_time;
    uint64_t execution_count;
} retryix_kernel_internal_t;

// 為了避免與 retryix_compute_backend.h 衝突，使用不同的名稱
typedef retryix_kernel_internal_t retryix_kernel_t;

#endif // RETRYIX_KERNEL_TYPE_DEFINED

// ===== Kernel 管理器結構體 =====
typedef struct {
    retryix_kernel_t kernels[RETRYIX_MAX_KERNELS];
    int kernel_count;
    void* cl_context;
    void* cl_device;
    void* cl_command_queue;
    int is_initialized;
} retryix_kernel_manager_t;

// ===== Queue 屬性設置 =====
/**
 * @brief 設定 Kernel 管理器的 queue 屬性
 * @param manager Kernel 管理器
 * @param props cl_queue_properties 指針
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_set_queue_props(
    retryix_kernel_manager_t* manager,
    const cl_queue_properties* props
);

// ===== Kernel 管理器初始化 =====
/**
 * @brief 初始化 Kernel 管理器
 * @param manager Kernel 管理器
 * @param device 目標設備
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_manager_init(
    retryix_kernel_manager_t* manager,
    const retryix_device_t* device
);

/**
 * @brief 清理 Kernel 管理器
 * @param manager Kernel 管理器
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_manager_cleanup(
    retryix_kernel_manager_t* manager
);

// ===== Kernel 創建與編譯 =====
/**
 * @brief 從源代碼創建 Kernel
 * @param manager Kernel 管理器
 * @param kernel_name Kernel 名稱
 * @param source_code 源代碼
 * @param build_options 編譯選項（可選）
 * @param kernel_id 創建的 Kernel ID 輸出
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_create_from_source(
    retryix_kernel_manager_t* manager,
    const char* kernel_name,
    const char* source_code,
    const char* build_options,
    int* kernel_id
);

/**
 * @brief 從檔案載入並創建 Kernel
 * @param manager Kernel 管理器
 * @param kernel_name Kernel 名稱
 * @param source_file 源代碼檔案路徑
 * @param build_options 編譯選項（可選）
 * @param kernel_id 創建的 Kernel ID 輸出
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_create_from_file(
    retryix_kernel_manager_t* manager,
    const char* kernel_name,
    const char* source_file,
    const char* build_options,
    int* kernel_id
);

/**
 * @brief 編譯 Kernel
 * @param manager Kernel 管理器
 * @param kernel_id Kernel ID
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_compile(
    retryix_kernel_manager_t* manager,
    int kernel_id
);

/**
 * @brief 獲取 Kernel 編譯日誌
 * @param manager Kernel 管理器
 * @param kernel_id Kernel ID
 * @param build_log 編譯日誌輸出緩衝區
 * @param max_log_len 最大日誌長度
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_get_build_log(
    retryix_kernel_manager_t* manager,
    int kernel_id,
    char* build_log,
    size_t max_log_len
);

// ===== Kernel 參數設置 =====
/**
 * @brief 設置 Kernel 緩衝區參數
 * @param manager Kernel 管理器
 * @param kernel_id Kernel ID
 * @param arg_index 參數索引
 * @param buffer_ptr 緩衝區指針
 * @param buffer_size 緩衝區大小
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_set_buffer_arg(
    retryix_kernel_manager_t* manager,
    int kernel_id,
    int arg_index,
    void* buffer_ptr,
    size_t buffer_size
);

/**
 * @brief 設置 Kernel SVM 指針參數
 * @param manager Kernel 管理器
 * @param kernel_id Kernel ID
 * @param arg_index 參數索引
 * @param svm_ptr SVM 指針
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_set_svm_arg(
    retryix_kernel_manager_t* manager,
    int kernel_id,
    int arg_index,
    void* svm_ptr
);

/**
 * @brief 設置 Kernel 標量參數
 * @param manager Kernel 管理器
 * @param kernel_id Kernel ID
 * @param arg_index 參數索引
 * @param arg_type 參數類型
 * @param value 參數值指針
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_set_scalar_arg(
    retryix_kernel_manager_t* manager,
    int kernel_id,
    int arg_index,
    retryix_kernel_arg_type_t arg_type,
    const void* value
);

/**
 * @brief 設置 Kernel 本地記憶體參數
 * @param manager Kernel 管理器
 * @param kernel_id Kernel ID
 * @param arg_index 參數索引
 * @param local_mem_size 本地記憶體大小
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_set_local_arg(
    retryix_kernel_manager_t* manager,
    int kernel_id,
    int arg_index,
    size_t local_mem_size
);

// ===== Kernel 執行 =====
/**
 * @brief 執行 Kernel
 * @param manager Kernel 管理器
 * @param kernel_id Kernel ID
 * @param config 執行配置
 * @param wait_for_completion 是否等待執行完成
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_execute(
    retryix_kernel_manager_t* manager,
    int kernel_id,
    const retryix_kernel_config_t* config,
    int wait_for_completion
);

/**
 * @brief 執行 Kernel（簡化版本）
 * @param manager Kernel 管理器
 * @param kernel_id Kernel ID
 * @param global_work_size 全域工作大小
 * @param local_work_size 本地工作大小（可選）
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_execute_1d(
    retryix_kernel_manager_t* manager,
    int kernel_id,
    size_t global_work_size,
    size_t local_work_size
);

/**
 * @brief 等待所有 Kernel 執行完成
 * @param manager Kernel 管理器
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_wait_all(
    retryix_kernel_manager_t* manager
);

/* Demo kernels were previously declared here. They are internal-only and
    have been moved to include/internal/retryix_demos.h. If you need to expose
    them for testing, define RETRYIX_DEMOS_PUBLIC before including public headers.
*/

#if defined(RETRYIX_DEMOS_PUBLIC) && RETRYIX_DEMOS_PUBLIC
/* If enabled, include the internal demos header which declares demo functions. */
#include "internal/retryix_demos.h"
#endif

// ===== Kernel 查詢與統計 =====
/**
 * @brief 根據名稱查找 Kernel
 * @param manager Kernel 管理器
 * @param kernel_name Kernel 名稱
 * @param kernel_id Kernel ID 輸出
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_find_by_name(
    retryix_kernel_manager_t* manager,
    const char* kernel_name,
    int* kernel_id
);

/**
 * @brief 獲取 Kernel 執行統計
 * @param manager Kernel 管理器
 * @param kernel_id Kernel ID
 * @param execution_count 執行次數輸出
 * @param total_time 總執行時間輸出（秒）
 * @param average_time 平均執行時間輸出（秒）
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_get_statistics(
    retryix_kernel_manager_t* manager,
    int kernel_id,
    uint64_t* execution_count,
    double* total_time,
    double* average_time
);

/**
 * @brief 重置 Kernel 統計
 * @param manager Kernel 管理器
 * @param kernel_id Kernel ID
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_reset_statistics(
    retryix_kernel_manager_t* manager,
    int kernel_id
);

// ===== Kernel 模板系統 =====
/**
 * @brief 註冊 Kernel 模板
 * @param manager Kernel 管理器
 * @param template_name 模板名稱
 * @param kernel_name Kernel 名稱
 * @param source_template 源代碼模板
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_register_template(
    retryix_kernel_manager_t* manager,
    const char* template_name,
    const char* kernel_name,
    const char* source_template
);

/**
 * @brief 從模板實例化 Kernel
 * @param manager Kernel 管理器
 * @param template_name 模板名稱
 * @param instance_name 實例名稱
 * @param parameters 模板參數（鍵值對字符串）
 * @param kernel_id 創建的 Kernel ID 輸出
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_kernel_instantiate_template(
    retryix_kernel_manager_t* manager,
    const char* template_name,
    const char* instance_name,
    const char* parameters,
    int* kernel_id
);

#ifdef __cplusplus
}
#endif

#endif // RETRYIX_KERNEL_H