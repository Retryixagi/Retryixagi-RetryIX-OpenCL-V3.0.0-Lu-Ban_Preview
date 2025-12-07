#ifndef RETRYIX_H
#include "retryix.h"
#endif

#pragma once
// retryix_bridge_android.h - Android GPU驅動橋接
#ifndef RETRYIX_BRIDGE_ANDROID_H
#define RETRYIX_BRIDGE_ANDROID_H

#include "retryix_core.h"
#include "retryix_platform_android.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===== Android OpenCL驅動路徑 =====
#define RETRYIX_ANDROID_OPENCL_ADRENO_LIB    "/system/vendor/lib64/libOpenCL.so"
#define RETRYIX_ANDROID_OPENCL_ADRENO_LIB32  "/system/vendor/lib/libOpenCL.so"
#define RETRYIX_ANDROID_OPENCL_MALI_LIB      "/system/vendor/lib64/libmali.so"
#define RETRYIX_ANDROID_OPENCL_MALI_LIB32    "/system/vendor/lib/libmali.so"
#define RETRYIX_ANDROID_OPENCL_POWERVR_LIB   "/system/vendor/lib64/libPVROCL.so"
#define RETRYIX_ANDROID_OPENCL_POWERVR_LIB32 "/system/vendor/lib/libPVROCL.so"

// ===== Android Vulkan驅動路徑 =====
#define RETRYIX_ANDROID_VULKAN_ADRENO_LIB    "/system/vendor/lib64/libvulkan_adreno.so"
#define RETRYIX_ANDROID_VULKAN_MALI_LIB      "/system/vendor/lib64/libvulkan_mali.so"
#define RETRYIX_ANDROID_VULKAN_POWERVR_LIB   "/system/vendor/lib64/libvulkan_powervr.so"

// ===== Android特定橋接結構 =====
typedef struct {
    retryix_android_gpu_type_t gpu_type;
    char driver_path[256];
    void* driver_handle;
    bool is_loaded;

    // OpenCL函數指針
    void* clGetPlatformIDs_ptr;
    void* clGetDeviceIDs_ptr;
    void* clCreateContext_ptr;
    void* clCreateCommandQueue_ptr;
    void* clSVMAlloc_ptr;
    void* clSVMFree_ptr;

    // Vulkan函數指針 (如果支援)
    void* vkCreateInstance_ptr;
    void* vkEnumeratePhysicalDevices_ptr;
    void* vkGetPhysicalDeviceProperties_ptr;
} retryix_android_bridge_t;

// ===== Android GPU驅動橋接函數 =====

/**
 * 發現Android平台
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_android_platforms(
    retryix_platform_t* platforms,
    int max_platforms,
    int* platform_count
);

/**
 * 初始化Android GPU橋接
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_bridge_init(
    retryix_android_bridge_t* bridge
);

/**
 * 載入Adreno驅動 (Qualcomm)
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_load_adreno_driver(
    retryix_android_bridge_t* bridge
);

/**
 * 載入Mali驅動 (ARM)
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_load_mali_driver(
    retryix_android_bridge_t* bridge
);

/**
 * 載入PowerVR驅動 (Imagination)
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_load_powervr_driver(
    retryix_android_bridge_t* bridge
);

/**
 * 載入Tegra驅動 (NVIDIA)
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_load_tegra_driver(
    retryix_android_bridge_t* bridge
);

/**
 * 清理Android橋接
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_bridge_cleanup(
    retryix_android_bridge_t* bridge
);

/**
 * 動態檢測最佳GPU驅動
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_detect_best_driver(
    retryix_android_bridge_t* bridge
);

/**
 * 獲取Android GPU設備信息
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_get_gpu_device_info(
    const retryix_android_bridge_t* bridge,
    retryix_device_t* device
);

/**
 * Android OpenCL上下文創建
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_create_opencl_context(
    const retryix_android_bridge_t* bridge,
    void** context,
    void** device,
    void** queue
);

/**
 * 檢測Android SVM支援
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_check_opencl_svm_support(
    const retryix_android_bridge_t* bridge,
    bool* supports_coarse_grain,
    bool* supports_fine_grain,
    bool* supports_atomics
);

// ===== Android特定優化 =====

/**
 * Android GPU記憶體對齊優化
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_optimize_memory_alignment(
    const retryix_android_bridge_t* bridge,
    size_t* alignment_bytes
);

/**
 * Android GPU工作組大小優化
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_optimize_workgroup_size(
    const retryix_android_bridge_t* bridge,
    size_t* optimal_local_size
);

/**
 * Android GPU頻率和電源管理
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_gpu_power_management(
    const retryix_android_bridge_t* bridge,
    bool enable_high_performance
);

#ifdef __cplusplus
}
#endif

#endif // RETRYIX_BRIDGE_ANDROID_H

