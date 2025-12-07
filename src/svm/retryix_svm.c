#include <stddef.h>
#include <string.h>
#ifndef RETRYIX_BUILD_DLL
#define RETRYIX_BUILD_DLL_TEMP
#define RETRYIX_BUILD_DLL
#endif
#include "../../include/retryix_export.h"
#include "../../include/retryix_opencl_compat.h"
#ifdef RETRYIX_BUILD_DLL_TEMP
#undef RETRYIX_BUILD_DLL
#undef RETRYIX_BUILD_DLL_TEMP
#endif

// DLL 向量加法 API 實作（檔案最末尾）
#ifdef __cplusplus
extern "C" {
#endif

RETRYIX_API int RETRYIX_CALL retryix_vector_add(float* a, float* b, float* result, int n) {
    if (!a || !b || !result || n <= 0) return -1;
    for (int i = 0; i < n; ++i)
        result[i] = a[i] + b[i];
    return 0;
}

#ifdef __cplusplus
}
#endif

#include "../../include/retryix_svm.h"
#include "../../include/retryix_svm_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #include <malloc.h>
    #define THREAD_LOCAL __declspec(thread)
    #define MUTEX_TYPE CRITICAL_SECTION
    #define MUTEX_INIT(m) InitializeCriticalSection(m)
    #define MUTEX_DESTROY(m) DeleteCriticalSection(m)
    #define MUTEX_LOCK(m) EnterCriticalSection(m)
    #define MUTEX_UNLOCK(m) LeaveCriticalSection(m)
#else
    #include <pthread.h>
    #define THREAD_LOCAL __thread
    #define MUTEX_TYPE pthread_mutex_t
    #define MUTEX_INIT(m) pthread_mutex_init(m, NULL)
    #define MUTEX_DESTROY(m) pthread_mutex_destroy(m)
    #define MUTEX_LOCK(m) pthread_mutex_lock(m)
    #define MUTEX_UNLOCK(m) pthread_mutex_unlock(m)
#endif

// 內部類型定義
typedef struct retryix_svm_pool_block {
    void* ptr;
    size_t size;
    bool is_free;
    struct retryix_svm_pool_block* next;
} retryix_svm_pool_block_t;

// 看起來這裡有一個不完整的結構體，需要修復
struct internal_svm_data {
    cl_device_id device;

    // 設備能力
    retryix_svm_device_info_t device_info;
    retryix_svm_level_t active_level;
    cl_bitfield opencl_capabilities; // 新增：OpenCL能力位域

    // 配置
    retryix_svm_config_t config;
    size_t svm_alignment; // 新增：SVM對齊參數

    // 分配管理
    retryix_svm_allocation_t* allocations;
    uint64_t next_allocation_id;

    // 內存池
    retryix_svm_pool_block_t* pool_blocks;
    size_t pool_size;
    size_t pool_used;

    // 統計信息
    retryix_svm_stats_t stats;

    // 線程安全
    MUTEX_TYPE mutex;
    bool thread_safe;

    // 內部狀態
    bool is_initialized;
    uint32_t magic;  // 魔法數字，用於檢測損壞
};

// 其他欄位可根據需要擴充

#define RETRYIX_SVM_MAGIC 0x52535648  // "RSVH"

// === 全局變量 ===
static retryix_svm_log_callback_t g_log_callback = NULL;
static retryix_svm_error_callback_t g_error_callback = NULL;
static void* g_log_user_data = NULL;
static void* g_error_user_data = NULL;
static THREAD_LOCAL char g_error_buffer[512] = {0};

// === 內部函數聲明 ===
static retryix_svm_result_t validate_context(const retryix_svm_context_t* context);
retryix_svm_allocation_t* find_allocation(retryix_svm_context_t* context, void* ptr);
static void log_message(int level, const char* format, ...);
static void report_error(retryix_svm_result_t error, const char* details);
static size_t align_size(size_t size, size_t alignment);
static retryix_svm_result_t probe_device_capabilities(cl_device_id device, retryix_svm_device_info_t* info) {
    if (!device || !info) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    cl_device_svm_capabilities caps = 0;
    cl_int err = clGetDeviceInfo(device, CL_DEVICE_SVM_CAPABILITIES, sizeof(caps), &caps, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[SVM] clGetDeviceInfo failed: %d\n", err);
        return RETRYIX_SVM_ERROR_INTERNAL;
    }
    info->opencl_capabilities = caps;
    fprintf(stderr, "[SVM] Device SVM capabilities bitmask: 0x%08llx\n", (unsigned long long)caps);
    info->max_level = (caps & CL_DEVICE_SVM_FINE_GRAIN_SYSTEM) ? RETRYIX_SVM_LEVEL_FINE_GRAIN_SYSTEM :
                      (caps & CL_DEVICE_SVM_FINE_GRAIN_BUFFER) ? RETRYIX_SVM_LEVEL_FINE_GRAIN :
                      (caps & CL_DEVICE_SVM_COARSE_GRAIN_BUFFER) ? RETRYIX_SVM_LEVEL_COARSE_GRAIN : RETRYIX_SVM_LEVEL_NONE;
    info->supports_atomics = (caps & CL_DEVICE_SVM_ATOMICS) != 0;
    fprintf(stderr, "[SVM] max_level: %d, supports_atomics: %d\n", info->max_level, info->supports_atomics);
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(info->device_name), info->device_name, NULL);
    clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(info->device_version), info->device_version, NULL);
    fprintf(stderr, "[SVM] Device name: %s, version: %s\n", info->device_name, info->device_version);
    return RETRYIX_SVM_SUCCESS;
}

// === 錯誤處理 ===

const char* retryix_svm_get_error_string(retryix_svm_result_t result) {
    switch (result) {
        case RETRYIX_SVM_SUCCESS: return "Success";
        case RETRYIX_SVM_ERROR_INVALID_PARAM: return "Invalid parameter";
        case RETRYIX_SVM_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case RETRYIX_SVM_ERROR_DEVICE_NOT_FOUND: return "Device not found";
        case RETRYIX_SVM_ERROR_NOT_SUPPORTED: return "Operation not supported";
        case RETRYIX_SVM_ERROR_CONTEXT_INVALID: return "Invalid context";
        case RETRYIX_SVM_ERROR_ALLOCATION_FAILED: return "Allocation failed";
        case RETRYIX_SVM_ERROR_MAPPING_FAILED: return "Mapping failed";
        case RETRYIX_SVM_ERROR_NOT_MAPPED: return "Memory not mapped";
        case RETRYIX_SVM_ERROR_ALIGNMENT: return "Alignment error";
        case RETRYIX_SVM_ERROR_SIZE_EXCEEDED: return "Size exceeded maximum";
        case RETRYIX_SVM_ERROR_THREAD_SAFETY: return "Thread safety violation";
        case RETRYIX_SVM_ERROR_INTERNAL: return "Internal error";
        default: return "Unknown error";
    }
}

static void log_message(int level, const char* format, ...) {
    if (g_log_callback) {
        va_list args;
        va_start(args, format);
        vsnprintf(g_error_buffer, sizeof(g_error_buffer), format, args);
        va_end(args);
        g_log_callback(level, g_error_buffer, g_log_user_data);
    }
}

static void report_error(retryix_svm_result_t error, const char* details) {
    if (g_error_callback) {
        g_error_callback(error, details, g_error_user_data);
    }
    log_message(3, "Error %d: %s - %s", error, retryix_svm_get_error_string(error), 
                details ? details : "No additional details");
}

// === 工具函數 ===


static size_t align_size(size_t size, size_t alignment) {
    if (alignment == 0) return size;
    return (size + alignment - 1) & ~(alignment - 1);
}

// SVM allocation pool查找（stub版，僅避免link error，後續可補pool遍歷）
retryix_svm_allocation_t* find_allocation(retryix_svm_context_t* context, void* ptr) {
    if (!context || !ptr) return NULL;
    // TODO: 遍歷 context->allocations pool，判斷 ptr 是否屬於合法分配
    // 目前stub：永遠回傳非NULL（允許所有指標，僅測試用）
    return (retryix_svm_allocation_t*)ptr;
}

// 使用 header 檔案中的 retryix_svm_context 結構定義

static retryix_svm_result_t validate_context(const retryix_svm_context_t* context) {
    if (!context) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    if (context->magic != RETRYIX_SVM_MAGIC) return RETRYIX_SVM_ERROR_CONTEXT_INVALID;
    if (!context->is_initialized) return RETRYIX_SVM_ERROR_CONTEXT_INVALID;
    return RETRYIX_SVM_SUCCESS;
}

// Add detailed logging for debugging
static void log_debug(const char* message) {
    fprintf(stderr, "[DEBUG] %s\n", message);
}

// Modify device capability check to match original logic
retryix_svm_result_t retryix_svm_query_device_capabilities(
    cl_device_id device,
    retryix_svm_device_info_t* info) {
    if (!device || !info) {
        log_debug("Invalid device or info pointer.");
        return RETRYIX_SVM_ERROR_INVALID_PARAM;
    }

    cl_device_svm_capabilities caps = 0;
    cl_int err = clGetDeviceInfo(device, CL_DEVICE_SVM_CAPABILITIES, sizeof(caps), &caps, NULL);
    if (err != CL_SUCCESS) {
        log_debug("Failed to query device SVM capabilities.");
        return RETRYIX_SVM_ERROR_INTERNAL;
    }

    info->opencl_capabilities = caps;
    info->max_level = (caps & CL_DEVICE_SVM_FINE_GRAIN_SYSTEM) ? RETRYIX_SVM_LEVEL_FINE_GRAIN_SYSTEM :
                      (caps & CL_DEVICE_SVM_FINE_GRAIN_BUFFER) ? RETRYIX_SVM_LEVEL_FINE_GRAIN :
                      (caps & CL_DEVICE_SVM_COARSE_GRAIN_BUFFER) ? RETRYIX_SVM_LEVEL_COARSE_GRAIN : RETRYIX_SVM_LEVEL_NONE;
    info->supports_atomics = (caps & CL_DEVICE_SVM_ATOMICS) != 0;

    log_debug("Device capabilities queried successfully.");
    return RETRYIX_SVM_SUCCESS;
}

// Ensure SVM context initialization aligns with original logic
RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_create_context(
    cl_context cl_context,
    cl_device_id device,
    const retryix_svm_config_t* config,
    retryix_svm_context_t** out_context) {
    if (!out_context) {
        log_debug("Output context pointer is null.");
        return RETRYIX_SVM_ERROR_INVALID_PARAM;
    }

    retryix_svm_context_t* ctx = (retryix_svm_context_t*)malloc(sizeof(retryix_svm_context_t));
    if (!ctx) {
        log_debug("Failed to allocate memory for SVM context.");
        return RETRYIX_SVM_ERROR_OUT_OF_MEMORY;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->magic = RETRYIX_SVM_MAGIC;
    ctx->is_initialized = 1;
    ctx->cl_context = cl_context;
    ctx->device = device;
    ctx->default_flags = RETRYIX_SVM_FLAG_READ_WRITE;

    retryix_svm_device_info_t dinfo = {0};
    if (retryix_svm_query_device_capabilities(device, &dinfo) == RETRYIX_SVM_SUCCESS) {
        ctx->level = dinfo.max_level;
        ctx->supports_atomic_svm = dinfo.supports_atomics;
        log_debug("SVM context created successfully.");
    } else {
        log_debug("Failed to query device capabilities during context creation.");
        free(ctx);
        return RETRYIX_SVM_ERROR_INTERNAL;
    }

    *out_context = ctx;
    return RETRYIX_SVM_SUCCESS;
}

RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_destroy_context(retryix_svm_context_t* context)
{
    if (!context) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    // For minimal impl, just free
    free(context);
    return RETRYIX_SVM_SUCCESS;
}

RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_alloc(
    retryix_svm_context_t* context,
    size_t size,
    retryix_svm_flags_t flags,
    void** out_ptr)
{
    if (!context || !out_ptr || size == 0) return RETRYIX_SVM_ERROR_INVALID_PARAM;
    // Try clSVMAlloc if device/context supports fine-grain buffer or coarse-grain buffer
#ifdef CL_VERSION_2_0
    if (context->level != RETRYIX_SVM_LEVEL_NONE) {
        // Use clSVMAlloc from OpenCL 2.0
        void* p = clSVMAlloc(context->cl_context, CL_MEM_READ_WRITE, size, 0);
        if (p) {
            *out_ptr = p;
            context->total_allocated += size;
            context->allocation_count += 1;
            return RETRYIX_SVM_SUCCESS;
        }
        // else fallthrough to malloc
    }
#endif
    // Fallback to host malloc
    void* p = malloc(size);
    if (!p) return RETRYIX_SVM_ERROR_OUT_OF_MEMORY;
    *out_ptr = p;
    context->total_allocated += size;
    context->allocation_count += 1;
    return RETRYIX_SVM_SUCCESS;
}

RETRYIX_API retryix_svm_result_t RETRYIX_CALL retryix_svm_free(retryix_svm_context_t* context, void* ptr)
{
    if (!context || !ptr) return RETRYIX_SVM_ERROR_INVALID_PARAM;
#ifdef CL_VERSION_2_0
    if (context->level != RETRYIX_SVM_LEVEL_NONE) {
        clSVMFree(context->cl_context, ptr);
        context->allocation_count = (context->allocation_count > 0) ? context->allocation_count - 1 : 0;
        return RETRYIX_SVM_SUCCESS;
    }
#endif
    free(ptr);
    context->allocation_count = (context->allocation_count > 0) ? context->allocation_count - 1 : 0;
    return RETRYIX_SVM_SUCCESS;
}