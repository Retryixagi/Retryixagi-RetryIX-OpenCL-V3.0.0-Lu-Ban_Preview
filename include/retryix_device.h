
#ifndef RETRYIX_DEVICE_H
#define RETRYIX_DEVICE_H

#include <stddef.h>
#include "retryix_opencl_compat.h"
#include "retryix_core.h"

// OpenCL 類型由 retryix_opencl_compat.h 統一定義

// 檢查是否有 OpenCL 2.0+ 的 cl_queue_properties
#ifndef CL_VERSION_2_0
  typedef void* cl_queue_properties;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ===== 設備能力位標誌 =====
typedef enum {
    RETRYIX_CAP_NONE = 0x00000000,
    RETRYIX_CAP_SVM_COARSE = 0x00000001,
    RETRYIX_CAP_SVM_FINE = 0x00000002,
    RETRYIX_CAP_ATOMIC_INT32 = 0x00000004,
    RETRYIX_CAP_ATOMIC_INT64 = 0x00000008,
    RETRYIX_CAP_GL_SHARING = 0x00000010,
    RETRYIX_CAP_IMAGE_SUPPORT = 0x00000020,
    RETRYIX_CAP_PIPES = 0x00000040,
    RETRYIX_CAP_DEVICE_ENQUEUE = 0x00000080
} retryix_device_capabilities_t;

// ===== 平台結構體 =====
// retryix_platform_t struct 已統一定義於 retryix_core.h，請直接 include 並使用。

// ===== 設備結構體 =====
typedef struct {
    uint32_t struct_size;    // = sizeof(retryix_device_t)
    uint32_t struct_version; // = 0x00010000
    char name[RETRYIX_MAX_NAME_LEN];
    char vendor[RETRYIX_MAX_NAME_LEN];
    char version[RETRYIX_MAX_VERSION_LEN];
    char driver_version[RETRYIX_MAX_VERSION_LEN];
    char extensions[RETRYIX_MAX_EXTENSIONS_LEN];

    cl_device_id id;
    cl_platform_id platform_id;
    cl_device_type type;

    // 硬體信息
    uint32_t compute_units;
    uint32_t max_frequency;
    cl_ulong global_memory;
    cl_ulong local_memory;
    size_t max_work_group_size;
    uint32_t max_work_item_dimensions;
    size_t max_work_item_sizes[3];

    // 能力標誌
    retryix_device_capabilities_t capabilities;
    cl_bitfield svm_capabilities;

    // 性能評分（0-100）
    float performance_score;

    // 可用性狀態
    int is_available;
    int is_preferred;
    // 自訂硬體標記
    uint32_t custom_flags; // 例如 RX5000 = 0x01, CUDA = 0x02, ROCm = 0x04, oneAPI = 0x08
    int is_amd_rx5000;     // 1=RX5000系列, 0=其他
} retryix_device_t;

// 其餘函數聲明保持不變...
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

#ifdef __cplusplus
}
#endif

#endif // RETRYIX_DEVICE_H