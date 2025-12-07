#ifndef RETRYIX_CORE_H
#define RETRYIX_CORE_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "retryix_export.h"

// ===== OpenCL 類型模擬 - 完全不依賴 OpenCL SDK =====
// 所有 OpenCL 類型由 retryix_opencl_compat.h 統一定義
// 這裡不再重複定義,避免衝突

// ===== 錯誤碼定義 =====
typedef enum {
    RETRYIX_SUCCESS = 0,
    RETRYIX_ERROR_NULL_PTR = -1,
    RETRYIX_ERROR_NO_DEVICE = -2,
    RETRYIX_ERROR_NO_PLATFORM = -3,
    RETRYIX_ERROR_OPENCL = -4,
    RETRYIX_ERROR_BUFFER_TOO_SMALL = -5,
    RETRYIX_ERROR_FILE_IO = -6,
    RETRYIX_ERROR_SVM_NOT_SUPPORTED = -7,
    RETRYIX_ERROR_INVALID_PARAMETER = -8,
    RETRYIX_ERROR_INVALID_DEVICE = -9,
    RETRYIX_ERROR_OUT_OF_MEMORY = -10,
    RETRYIX_ERROR_KERNEL_COMPILATION = -11,
    RETRYIX_ERROR_ATOMIC_NOT_SUPPORTED = -12,
    RETRYIX_ERROR_NOT_INITIALIZED = -13,
    RETRYIX_ERROR_FILE_NOT_FOUND = -14,
    RETRYIX_ERROR_COMPILATION_FAILED = -15,
    RETRYIX_ERROR_NOT_FOUND = -16,
    RETRYIX_ERROR_OPENCL_ERROR = -17,
    RETRYIX_ERROR_UNKNOWN = -100
} retryix_result_t;

typedef struct retryix_platform_t {
    char vendor[64];    /* e.g., "Intel"/"AMD"/"NVIDIA" */
    char name[128];     /* device/platform friendly name */
    char version[128];  /* driver/runtime/version summary */
    char profile[64];   /* "LevelZero"/"OpenCL"/"CUDA"/"ROCm"/"CPU" */
    char extensions[2048]; /* OpenCL extensions */
    void* id;           /* cl_platform_id, opaque for portability */
    int  device_count;  /* how many devices are represented */
} retryix_platform_t;

// ===== 常量定義 =====
#define RETRYIX_MAX_PLATFORMS       16
#define RETRYIX_MAX_DEVICES         64
#define RETRYIX_MAX_NAME_LEN        256
#define RETRYIX_MAX_VERSION_LEN     128
#define RETRYIX_MAX_EXTENSIONS_LEN  2048
#define RETRYIX_MAX_JSON_OUTPUT     8192

// ===== 版本信息 =====
#define RETRYIX_VERSION_MAJOR       2
#define RETRYIX_VERSION_MINOR       0
#define RETRYIX_VERSION_PATCH       4
#define RETRYIX_VERSION_STRING      "3.0.0"

// ===== 核心初始化與清理 =====
/**
 * @brief 初始化 RetryIX 系統
 * @return RETRYIX_SUCCESS 成功，其他值為錯誤碼
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_initialize(void);

/**
 * @brief 清理 RetryIX 系統資源
 */
RETRYIX_API void RETRYIX_CALL retryix_cleanup(void);

/**
 * @brief 獲取 RetryIX 版本信息
 * @param major_version 主版本號輸出
 * @param minor_version 次版本號輸出
 * @param patch_version 補丁版本號輸出
 * @return 版本字符串
 */
RETRYIX_API const char* RETRYIX_CALL retryix_get_version(
    int* major_version, 
    int* minor_version, 
    int* patch_version
);

/**
 * @brief 獲取錯誤信息字符串
 * @param error_code 錯誤碼
 * @return 錯誤描述字符串
 */
RETRYIX_API const char* RETRYIX_CALL retryix_get_error_string(retryix_result_t error_code);

#ifdef __cplusplus
}
#endif

#endif // RETRYIX_CORE_H