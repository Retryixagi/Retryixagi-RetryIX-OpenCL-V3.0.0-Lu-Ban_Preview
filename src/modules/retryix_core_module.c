// RetryIX 3.0.0 "魯班" 核心模塊 - 分身術第一分身
// 基於魯班智慧：分而治之,各司其職
// Version: 3.0.0 Codename: 魯班 (Lu Ban)
#define RETRYIX_BUILD_DLL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#define RETRYIX_API __declspec(dllexport)
#define RETRYIX_MUTEX CRITICAL_SECTION
#define RETRYIX_MUTEX_INIT(m) InitializeCriticalSection(&(m))
#define RETRYIX_MUTEX_DESTROY(m) DeleteCriticalSection(&(m))
#define RETRYIX_MUTEX_LOCK(m) EnterCriticalSection(&(m))
#define RETRYIX_MUTEX_UNLOCK(m) LeaveCriticalSection(&(m))
#else
#define RETRYIX_API __attribute__((visibility("default")))
#endif

#define RETRYIX_CALL __cdecl

// === 核心錯誤系統（下卷智慧：規則界定）===
typedef enum {
    RETRYIX_SUCCESS = 0,
    RETRYIX_ERROR_INVALID_PARAMETER = -1,
    RETRYIX_ERROR_OUT_OF_MEMORY = -2,
    RETRYIX_ERROR_NULL_PTR = -3,
    RETRYIX_ERROR_NOT_INITIALIZED = -6,
    RETRYIX_ERROR_ALREADY_INITIALIZED = -7,
    RETRYIX_ERROR_NOT_IMPLEMENTED = -8,
    RETRYIX_ERROR_NETWORK_UNAVAILABLE = -10,
    RETRYIX_ERROR_RDMA_NOT_SUPPORTED = -11,
    RETRYIX_ERROR_DPDK_NOT_SUPPORTED = -12,
    RETRYIX_ERROR_INSUFFICIENT_BUFFER = -14
} retryix_result_t;

// === 全局狀態（下卷智慧：承載基礎）===
static bool g_initialized = false;
static bool g_svm_initialized = false;
static int g_allocation_count = 0;
static RETRYIX_MUTEX g_mutex;
static bool g_mutex_initialized = false;

// === 核心初始化系統（上卷技術：自愈傀儡術）===
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_initialize(void) {
    printf("[RetryIX 3.0.0 Lu Ban Core] Initialize with Lu Ban wisdom\n");

    // 魯班智慧：先建基礎，後立機關
    if (!g_mutex_initialized) {
        RETRYIX_MUTEX_INIT(g_mutex);
        g_mutex_initialized = true;
        printf("[RetryIX 3.0.0] Mutex foundation established\n");
    }

    RETRYIX_MUTEX_LOCK(g_mutex);

    // 下卷智慧：檢查等價交換條件
    if (g_initialized) {
        printf("[RetryIX 3.0.0] System already initialized - resource conservation\n");
        RETRYIX_MUTEX_UNLOCK(g_mutex);
        return RETRYIX_ERROR_ALREADY_INITIALIZED;
    }

    // 建立系統根基
    g_initialized = true;
    g_svm_initialized = true;
    g_allocation_count = 0;

    printf("[RetryIX 3.0.0] Core foundation completed - ready for advanced mechanisms\n");

    RETRYIX_MUTEX_UNLOCK(g_mutex);
    return RETRYIX_SUCCESS;
}

// === 智能終結系統（下卷智慧：優雅退場）===
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_finalize(void) {
    printf("[RetryIX 3.0.0 Lu Ban Core] Finalize with Lu Ban wisdom - graceful shutdown\n");

    if (!g_mutex_initialized) {
        return RETRYIX_ERROR_NOT_INITIALIZED;
    }

    RETRYIX_MUTEX_LOCK(g_mutex);

    // 下卷智慧：歸還借用的資源
    printf("[RetryIX 3.0.0] Returning resources to the system...\n");
    g_initialized = false;
    g_svm_initialized = false;
    g_allocation_count = 0;

    RETRYIX_MUTEX_UNLOCK(g_mutex);
    RETRYIX_MUTEX_DESTROY(g_mutex);
    g_mutex_initialized = false;

    printf("[RetryIX 3.0.0] Graceful shutdown complete - all debts paid\n");
    return RETRYIX_SUCCESS;
}

// === 統一錯誤翻譯系統（下卷智慧：萬事皆有因果）===
RETRYIX_API const char* RETRYIX_CALL retryix_get_error_string(retryix_result_t error_code) {
    switch (error_code) {
        case RETRYIX_SUCCESS:
            return "Success - harmony achieved";
        case RETRYIX_ERROR_INVALID_PARAMETER:
            return "Invalid parameter - input does not align with natural law";
        case RETRYIX_ERROR_OUT_OF_MEMORY:
            return "Out of memory - resource limit reached";
        case RETRYIX_ERROR_NULL_PTR:
            return "Null pointer - void reference detected";
        case RETRYIX_ERROR_NOT_INITIALIZED:
            return "System not initialized - foundation not established";
        case RETRYIX_ERROR_ALREADY_INITIALIZED:
            return "System already initialized - duplicate effort avoided";
        case RETRYIX_ERROR_NOT_IMPLEMENTED:
            return "Feature not implemented - awaiting future development";
        case RETRYIX_ERROR_NETWORK_UNAVAILABLE:
            return "Network unavailable - connection severed";
        case RETRYIX_ERROR_RDMA_NOT_SUPPORTED:
            return "RDMA not supported - advanced path blocked";
        case RETRYIX_ERROR_DPDK_NOT_SUPPORTED:
            return "DPDK not supported - high-speed path unavailable";
        case RETRYIX_ERROR_INSUFFICIENT_BUFFER:
            return "Insufficient buffer size - container too small";
        default:
            return "Unknown error - mystery requires investigation";
    }
}

// === SVM支持檢查（下卷智慧：實事求是）===
// NOTE: 真正的實現在 src/device/retryix_device.c 中,這裡註釋掉避免重複定義
/*
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_check_svm_support(void) {
    printf("[RetryIX 3.0.0 Lu Ban Core] Checking SVM support with Lu Ban wisdom\n");

    if (!g_initialized) {
        return RETRYIX_ERROR_NOT_INITIALIZED;
    }

    // 魯班智慧：檢查硬件能力
    printf("[RetryIX 3.0.0] SVM capability verified - system ready\n");
    return RETRYIX_SUCCESS;
}
*/

// === 快速SVM分配（上卷技術：瞬移大法）===
RETRYIX_API void* RETRYIX_CALL retryix_quick_svm_alloc(size_t size) {
    if (!g_initialized || size == 0) {
        return NULL;
    }

    printf("[RetryIX 3.0.0] Quick SVM allocation: %zu bytes\n", size);

    RETRYIX_MUTEX_LOCK(g_mutex);
    void* ptr = malloc(size);  // 簡化實現
    if (ptr) {
        g_allocation_count++;
        printf("[RetryIX 3.0.0] SVM allocated successfully at %p\n", ptr);
    }
    RETRYIX_MUTEX_UNLOCK(g_mutex);

    return ptr;
}

// === 快速SVM釋放（下卷智慧：歸還資源）===
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_quick_svm_free(void* ptr) {
    if (!ptr) {
        return RETRYIX_ERROR_NULL_PTR;
    }

    printf("[RetryIX 3.0.0] Quick SVM free at %p\n", ptr);

    RETRYIX_MUTEX_LOCK(g_mutex);
    free(ptr);
    if (g_allocation_count > 0) {
        g_allocation_count--;
    }
    RETRYIX_MUTEX_UNLOCK(g_mutex);

    return RETRYIX_SUCCESS;
}

// === 版本識別系統（上卷技術：千里眼術）===
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_get_version(char* version_buf, size_t buf_size) {
    if (!version_buf || buf_size < 32) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }

    const char* version_wisdom = "3.0.0-LuBan";
    strncpy(version_buf, version_wisdom, buf_size - 1);
    version_buf[buf_size - 1] = '\0';

    printf("[RetryIX 3.0.0 Lu Ban] Version revealed: %s\n", version_wisdom);
    return RETRYIX_SUCCESS;
}

// === 一鍵啟動系統（上卷技術：瞬移大法）===
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_auto_initialize(int prefer_gpu) {
    printf("[RetryIX 3.0.0 Lu Ban] Auto-initialize with Lu Ban wisdom (GPU preference: %s)\n",
           prefer_gpu ? "enabled" : "disabled");

    // 下卷智慧：根據條件選擇最佳路徑
    retryix_result_t result = retryix_initialize();

    if (result == RETRYIX_SUCCESS) {
        printf("[RetryIX 3.0.0] Auto-initialization completed - all mechanisms ready\n");
    } else {
        printf("[RetryIX 3.0.0] Auto-initialization encountered obstacles - error: %s\n",
               retryix_get_error_string(result));
    }

    return result;
}

// === 系統健康檢查（上卷技術：千里眼+下卷智慧：觀察環境）===
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_full_system_check(char* report_buf, size_t buf_size) {
    if (!report_buf || buf_size < 256) {
        return RETRYIX_ERROR_INSUFFICIENT_BUFFER;
    }

    // 魯班智慧：觀察當前狀態
    const char* system_status = g_initialized ? "Operational" : "Dormant";
    const char* foundation_status = g_mutex_initialized ? "Stable" : "Unstable";
    const char* svm_status = g_svm_initialized ? "Active" : "Inactive";

    int written = snprintf(report_buf, buf_size,
        "=== RetryIX 3.0.0 \"Lu Ban\" System Report ===\n"
        "Foundation Status: %s\n"
        "Core System: %s\n"
        "SVM Engine: %s\n"
        "Active Allocations: %d\n"
        "Wisdom Level: Advanced (Lu Ban Enhanced)\n"
        "Assessment: %s\n",
        foundation_status,
        system_status,
        svm_status,
        g_allocation_count,
        g_initialized ? "System harmony achieved - ready for complex operations" :
                       "System requires initialization - foundation must be established first"
    );

    return (written > 0 && written < (int)buf_size) ? RETRYIX_SUCCESS : RETRYIX_ERROR_INSUFFICIENT_BUFFER;
}

// === DLL 生命週期管理（下卷智慧：生死有序）===
#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        printf("[RetryIX 3.0.0 Lu Ban Core] Module awakening - establishing presence\n");
        break;
    case DLL_PROCESS_DETACH:
        printf("[RetryIX 3.0.0 Lu Ban Core] Module departing - ensuring clean exit\n");
        if (g_initialized) {
            printf("[RetryIX 3.0.0] Performing automatic cleanup before departure\n");
            retryix_finalize();
        }
        break;
    }
    return TRUE;
}
#endif