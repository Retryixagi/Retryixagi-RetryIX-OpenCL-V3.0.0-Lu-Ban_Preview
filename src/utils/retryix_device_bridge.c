// retryix_device_bridge.c - 設備橋接器實現
#include "retryix_unified_device_bridge.h"
#include "retryix_compute_backend.h"
#include <stdio.h>
#include <string.h>

static retryix_backend_t* g_primary_backend = NULL;
static int g_initialized = 0;

// 初始化設備橋接器
int retryix_initialize_device_bridge(int preferred_backend_kind) {
    (void)preferred_backend_kind; // 避免未使用警告
    
    if (g_initialized) {
        return 0; // 已經初始化
    }
    
    // TODO: 實際初始化邏輯
    g_initialized = 1;
    return 0;
}

// 獲取可用後端
int retryix_get_available_backends(int* out_kinds, size_t max_count) {
    if (!out_kinds || max_count == 0) {
        return 0;
    }
    
    // 簡化實現：只返回一個 AUTO 後端
    out_kinds[0] = RETRYIX_BACKEND_AUTO;
    return 1;
}

// 獲取特定後端
retryix_backend_t* retryix_get_backend(int backend_kind) {
    (void)backend_kind; // 避免未使用警告
    
    // 簡化實現：返回 NULL
    return NULL;
}

// 獲取主要後端
retryix_backend_t* retryix_get_primary_backend(void) {
    return g_primary_backend;
}

// 後端類型轉字串
const char* retryix_backend_kind_to_string(int kind) {
    switch (kind) {
        case RETRYIX_BACKEND_AUTO: return "Auto";
        case RETRYIX_BACKEND_OPENCL: return "OpenCL";
        case RETRYIX_BACKEND_CUDA: return "CUDA";
        case RETRYIX_BACKEND_HIP: return "HIP";
        case RETRYIX_BACKEND_LEVELZERO: return "Level Zero";
        case RETRYIX_BACKEND_VULKAN: return "Vulkan";
        default: return "Unknown";
    }
}

// 關閉設備橋接器
void retryix_shutdown_device_bridge(void) {
    g_primary_backend = NULL;
    g_initialized = 0;
}
