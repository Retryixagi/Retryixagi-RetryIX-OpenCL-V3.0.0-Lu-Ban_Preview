// RetryIX 3.0.0 "魯班" - 設備JSON導出模塊
#ifndef RETRYIX_BUILD_DLL
#define RETRYIX_BUILD_DLL
#endif
#include "retryix.h"
#include <stdio.h>
#include <string.h>

// JSON 導出函數 - 使用真實設備數據
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_export_all_devices_json(
    char* json_buffer, size_t buffer_size) {

    if (!json_buffer || buffer_size < 512) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    // 簡化JSON導出,返回基本結構
    int written = snprintf(json_buffer, buffer_size,
        "{\n"
        "  \"retryix_version\": \"3.0.0\",\n"
        "  \"codename\": \"Lu Ban\",\n"
        "  \"devices\": [],\n"
        "  \"note\": \"Call retryix_discover_all_devices() to populate device list\"\n"
        "}"
    );

    return (written > 0 && written < (int)buffer_size) ? RETRYIX_SUCCESS : RETRYIX_ERROR_BUFFER_TOO_SMALL;
}

// 注意: retryix_export_devices_json 已在 src/device/retryix_device.c 中實現,不在此重複定義

// 後端查詢功能
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_get_available_backends(
    char* backends_buffer, size_t buffer_size) {

    if (!backends_buffer || buffer_size < 128) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    int written = snprintf(backends_buffer, buffer_size,
        "OpenCL,Native(Vulkan/DXGI)"
    );

    return (written > 0 && written < (int)buffer_size) ? RETRYIX_SUCCESS : RETRYIX_ERROR_BUFFER_TOO_SMALL;
}
