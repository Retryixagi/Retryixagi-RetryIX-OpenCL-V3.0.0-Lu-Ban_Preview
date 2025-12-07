// retryix_api.c - RetryIX 公共 API 實現
#include "retryix.h" // 確保正確引入型別定義
#include "retryix_svm.h" // 確保 struct 定義可用
#include <string.h>

static retryix_system_state_t g_api_state = {0};

const retryix_system_state_t* retryix_get_system_state(void) {
    return &g_api_state;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_auto_initialize(int prefer_gpu) {
    (void)prefer_gpu; // Suppress unused parameter warning
    // TODO: Initialize core, devices, SVM, Kernel
    g_api_state.is_initialized = 1;
    g_api_state.device_count = 1;
    // Discover devices and choose the first available as best_device.
    // Retry-on-failure strategy: allow a configurable number of retries via
    // the environment variable RETRYIX_DISCOVERY_RETRIES (default 1 retry).
    retryix_device_t devs[RETRYIX_MAX_DEVICES];
    int dev_count = 0;
    int found = 0;
    int retries = 1; /* default: one retry (total attempts = retries + 1) */
    {
        const char* env = getenv("RETRYIX_DISCOVERY_RETRIES");
        if (env) {
            long v = strtol(env, NULL, 10);
            if (v >= 0 && v <= 10) retries = (int)v;
        }
    }
    int max_attempts = 1 + retries;
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        dev_count = 0;
        if (retryix_discover_all_devices(devs, RETRYIX_MAX_DEVICES, &dev_count) == RETRYIX_SUCCESS && dev_count > 0) {
            g_api_state.device_count = dev_count;
            g_api_state.best_device = devs[0];
            found = 1;
            break;
        }
        if (attempt + 1 < max_attempts) {
            /* small pause helps drivers that initialize asynchronously */
#if defined(_WIN32)
            Sleep(50);
#else
            struct timespec ts = {0, 50 * 1000000}; /* 50 ms */
            nanosleep(&ts, NULL);
#endif
        }
    }
    if (!found) {
        memset(&g_api_state.best_device, 0, sizeof(g_api_state.best_device));
        strcpy(g_api_state.best_device.name, "DummyDevice");
        g_api_state.device_count = 0;
    }
    
    // Initialize SVM context if not already allocated
    if (!g_api_state.svm_context) {
        // 模擬初始化 - 不使用 OpenCL API
        cl_device_id device = (cl_device_id)0x1000;  // 模擬設備 handle
        cl_context ctx = (cl_context)0x2000;          // 模擬上下文 handle
        
        if (ctx && device) {
            retryix_svm_config_t cfg = {0};
            retryix_svm_context_t* svm_ctx = NULL;
            if (retryix_svm_create_context(ctx, device, &cfg, &svm_ctx) == RETRYIX_SVM_SUCCESS) {
                g_api_state.svm_context = svm_ctx;
            }
        }
    }
    
    return RETRYIX_SUCCESS;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_get_best_device(retryix_device_t* device) {
    if (!device || !g_api_state.is_initialized) {
        return RETRYIX_ERROR_NULL_PTR;
    }
    *device = g_api_state.best_device;
    return RETRYIX_SUCCESS;
}

RETRYIX_API int RETRYIX_CALL retryix_is_svm_initialized(void) {
    return (g_api_state.svm_context != NULL) ? 1 : 0;
}

RETRYIX_API int RETRYIX_CALL retryix_get_svm_level(void) {
    if (!g_api_state.svm_context) return RETRYIX_SVM_LEVEL_NONE;
    return (int)g_api_state.svm_context->level;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_full_system_check(char* json_report, size_t max_report_len) {
    // TODO: 完整系統檢查並輸出 JSON 報告
    if (json_report && max_report_len > 0) {
        strncpy(json_report, "{\"status\":\"ok\",\"initialized\":true}", max_report_len - 1);
        json_report[max_report_len - 1] = '\0';
    }
    return RETRYIX_SUCCESS;
}

/* Demos are optional and should not be part of the core public surface by
 * default. Only compile this function when RETRYIX_DEMOS_PUBLIC is enabled.
 */
#if defined(RETRYIX_DEMOS_PUBLIC) && RETRYIX_DEMOS_PUBLIC
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_run_python_equivalent_demo(void) {
    // TODO: 執行 Python 等效示範
    return RETRYIX_SUCCESS;
}
#endif

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_quick_svm_alloc(size_t size, retryix_svm_allocation_t* allocation) {
    if (!allocation || size == 0) return RETRYIX_ERROR_NULL_PTR;

    allocation->ptr = NULL;
    allocation->size = size;
    allocation->actual_size = 0;
    allocation->flags = 0;
    allocation->level = RETRYIX_SVM_LEVEL_NONE;
    allocation->is_mapped = false;
    allocation->allocation_id = 0;

    if (g_api_state.svm_context) {
        void* out_ptr = NULL;
        retryix_svm_result_t r = retryix_svm_alloc(g_api_state.svm_context, size, RETRYIX_SVM_FLAG_READ_WRITE, &out_ptr);
    if (r == RETRYIX_SVM_SUCCESS && out_ptr) {
            allocation->ptr = out_ptr;
            allocation->actual_size = size;
            allocation->flags = RETRYIX_SVM_FLAG_READ_WRITE;
            allocation->level = g_api_state.svm_context->level;
            allocation->allocation_id = 1;
            return RETRYIX_SUCCESS;
        }
    }

    // Fallback
    allocation->ptr = malloc(size);
    if (!allocation->ptr) return RETRYIX_SVM_ERROR_OUT_OF_MEMORY;
    allocation->actual_size = size;
    allocation->flags = RETRYIX_SVM_FLAG_HOST_PTR;
    allocation->level = RETRYIX_SVM_LEVEL_EMULATED;
    allocation->allocation_id = 1;
    return RETRYIX_SUCCESS;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_quick_svm_free(retryix_svm_allocation_t* allocation) {
    if (!allocation) return RETRYIX_ERROR_NULL_PTR;
    if (!allocation->ptr) return RETRYIX_SUCCESS;

    if (g_api_state.svm_context) {
        retryix_svm_result_t r = retryix_svm_free(g_api_state.svm_context, allocation->ptr);
        if (r == RETRYIX_SVM_SUCCESS) {
            allocation->ptr = NULL;
            allocation->size = 0;
            allocation->actual_size = 0;
            allocation->allocation_id = 0;
            return RETRYIX_SUCCESS;
        }
    }

    free(allocation->ptr);
    allocation->ptr = NULL;
    allocation->size = 0;
    allocation->actual_size = 0;
    allocation->allocation_id = 0;
    return RETRYIX_SUCCESS;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_quick_kernel_run(const char* kernel_name, const char* source_code, size_t global_work_size, size_t local_work_size, retryix_kernel_arg_t* args, int arg_count) {
    // TODO: 快速核心執行
    (void)kernel_name; (void)source_code; (void)global_work_size; (void)local_work_size; (void)args; (void)arg_count;
    return RETRYIX_SUCCESS;
}

RETRYIX_API void RETRYIX_CALL retryix_api_cleanup(void) {
    // 清理 API 狀態
    g_api_state.is_initialized = 0;
    g_api_state.device_count = 0;
    g_api_state.svm_context = NULL;
}
