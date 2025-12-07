#pragma once
// retryix_platform_android.h - Android平台專用API和類型定義
#ifndef RETRYIX_PLATFORM_ANDROID_H
#define RETRYIX_PLATFORM_ANDROID_H

#include "retryix_core.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===== Android特定常量 =====
#define RETRYIX_ANDROID_MAX_GPU_DRIVERS 8
#define RETRYIX_ANDROID_MAX_SYSTEM_PROPS 32
#define RETRYIX_ANDROID_PROP_VALUE_MAX 256

// ===== Android GPU驅動類型 =====
typedef enum {
    RETRYIX_ANDROID_GPU_UNKNOWN = 0,
    RETRYIX_ANDROID_GPU_ADRENO,      // Qualcomm Adreno
    RETRYIX_ANDROID_GPU_MALI,        // ARM Mali
    RETRYIX_ANDROID_GPU_POWERVR,     // Imagination PowerVR
    RETRYIX_ANDROID_GPU_TEGRA,       // NVIDIA Tegra
    RETRYIX_ANDROID_GPU_VIVANTE,     // Vivante (NXP)
    RETRYIX_ANDROID_GPU_INTEL        // Intel HD Graphics (x86 Android)
} retryix_android_gpu_type_t;

// ===== Android設備信息結構 =====
typedef struct {
    retryix_android_gpu_type_t gpu_type;
    char vendor[64];                 // GPU供應商
    char renderer[128];              // GPU渲染器名稱
    char version[128];               // OpenGL ES版本
    char extensions[2048];           // 支援的擴展

    // Android系統信息
    int api_level;                   // Android API Level
    char board[64];                  // 主板型號
    char brand[64];                  // 品牌 (Samsung, Xiaomi, etc.)
    char model[64];                  // 型號
    char manufacturer[64];           // 製造商
    char product[64];                // 產品名稱

    // GPU能力
    bool supports_opencl;            // 支援OpenCL
    bool supports_vulkan;            // 支援Vulkan
    bool supports_gles_compute;      // 支援OpenGL ES Compute Shaders
    int max_compute_units;           // 最大計算單元數
    size_t max_work_group_size;      // 最大工作組大小
    size_t global_mem_size;          // 全局記憶體大小
    size_t local_mem_size;           // 本地記憶體大小
} retryix_android_device_info_t;

// ===== Android OpenCL驅動檢測 =====
typedef struct {
    char lib_path[256];              // OpenCL庫路徑
    char vendor[64];                 // 驅動供應商
    char version[64];                // 驅動版本
    bool is_available;               // 是否可用
    void* handle;                    // 動態庫句柄
} retryix_android_opencl_driver_t;

// ===== Android平台檢測函數 =====

/**
 * 檢測Android設備GPU類型
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_detect_gpu_type(
    retryix_android_gpu_type_t* gpu_type,
    retryix_android_device_info_t* device_info
);

/**
 * 獲取Android系統屬性
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_get_system_property(
    const char* key,
    char* value,
    size_t value_size
);

/**
 * 掃描可用的OpenCL驅動
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_scan_opencl_drivers(
    retryix_android_opencl_driver_t* drivers,
    int max_drivers,
    int* driver_count
);

/**
 * 載入Android OpenCL驅動
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_load_opencl_driver(
    const char* lib_path,
    void** driver_handle
);

/**
 * 檢測Adreno GPU能力 (Qualcomm)
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_detect_adreno_capabilities(
    retryix_android_device_info_t* device_info
);

/**
 * 檢測Mali GPU能力 (ARM)
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_detect_mali_capabilities(
    retryix_android_device_info_t* device_info
);

/**
 * 檢測PowerVR GPU能力 (Imagination)
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_detect_powervr_capabilities(
    retryix_android_device_info_t* device_info
);

/**
 * 初始化Android平台OpenCL環境
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_opencl_init(void);

/**
 * 清理Android平台OpenCL環境
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_opencl_cleanup(void);

/**
 * 獲取Android OpenGL ES信息
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_get_gles_info(
    retryix_android_device_info_t* device_info
);

/**
 * 檢查Android權限
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_check_permissions(void);

// ===== Android特定SVM支援 =====

/**
 * 檢測Android設備SVM能力
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_check_svm_support(
    bool* supports_svm,
    bool* supports_fine_grain,
    bool* supports_atomics
);

/**
 * Android專用SVM分配 (使用ion或ashmem)
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_svm_alloc(
    size_t size,
    void** ptr,
    bool use_ion_allocator
);

/**
 * Android專用SVM釋放
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_svm_free(void* ptr);

// ===== Android性能優化 =====

/**
 * 設置Android GPU性能模式
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_set_gpu_performance_mode(
    int performance_level  // 0=省電, 1=平衡, 2=性能
);

/**
 * 獲取Android GPU溫度和頻率信息
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_get_gpu_thermal_info(
    float* temperature_celsius,
    int* current_frequency_mhz,
    int* max_frequency_mhz
);

#ifdef __cplusplus
}
#endif

#endif // RETRYIX_PLATFORM_ANDROID_H