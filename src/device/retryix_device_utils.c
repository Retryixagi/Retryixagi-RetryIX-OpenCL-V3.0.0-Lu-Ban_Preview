// retryix_device_utils.c
#include "retryix_device.h"
#include <string.h>

// 平台/設備查詢 stub
extern RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_platforms(
    retryix_platform_t* platforms,
    int max_platforms,
    int* platform_count
);

extern RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_devices_for_platform(
    cl_platform_id platform_id,
    retryix_device_t* devices,
    int max_devices,
    int* device_count
);

extern RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_all_devices(
    retryix_device_t* devices,
    int max_devices,
    int* device_count
);

extern RETRYIX_API retryix_result_t RETRYIX_CALL retryix_select_best_device(
    const retryix_device_t* devices,
    int device_count,
    retryix_device_t* best_device
);

extern RETRYIX_API retryix_result_t RETRYIX_CALL retryix_filter_devices_by_type(
    const retryix_device_t* devices,
    int device_count,
    cl_device_type device_type,
    retryix_device_t* filtered_devices,
    int max_filtered,
    int* filtered_count
);

extern RETRYIX_API int RETRYIX_CALL retryix_device_supports_capability(
    const retryix_device_t* device,
    retryix_device_capabilities_t capability
);

extern RETRYIX_API retryix_result_t RETRYIX_CALL retryix_check_svm_support(
    const retryix_device_t* device,
    int* supports_coarse,
    int* supports_fine,
    int* supports_atomic
);

extern RETRYIX_API retryix_result_t RETRYIX_CALL retryix_check_atomic_support(
    const retryix_device_t* device,
    int* supports_int32,
    int* supports_int64
);

extern RETRYIX_API retryix_result_t RETRYIX_CALL retryix_export_devices_json(
    const retryix_device_t* devices,
    int device_count,
    char* json_output,
    size_t max_json_len
);

extern RETRYIX_API retryix_result_t RETRYIX_CALL retryix_save_devices_to_file(
    const retryix_device_t* devices,
    int device_count,
    const char* filename
);
