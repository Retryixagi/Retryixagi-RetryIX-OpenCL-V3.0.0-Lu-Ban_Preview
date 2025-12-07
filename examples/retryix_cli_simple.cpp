/*
 * RetryIX V7 Simple CLI - 正確對應實際 API
 * 只呼叫確實存在的函數
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// 動態載入函數指標
typedef int (*fn_initialize)(void);
typedef int (*fn_finalize)(void);
typedef int (*fn_get_version)(char*, size_t);
typedef const char* (*fn_strerror)(int);
typedef int (*fn_full_system_check)(char*, size_t);
typedef void* (*fn_svm_alloc)(size_t);
typedef int (*fn_svm_free)(void*);
typedef int (*fn_zerocopy_net_init)(void);
typedef int (*fn_bridge_initialize_universal)(void);
typedef int (*fn_atomic_compare_exchange_i32)(volatile int*, int, int, int*);
typedef int (*fn_bus_scheduler_init)(void);
typedef int (*fn_kernel_execute)(void);

static HMODULE g_dll = nullptr;
static fn_initialize retryix_initialize = nullptr;
static fn_finalize retryix_finalize = nullptr;
static fn_get_version retryix_get_version = nullptr;
static fn_strerror retryix_strerror = nullptr;
static fn_full_system_check retryix_full_system_check = nullptr;
static fn_svm_alloc retryix_svm_alloc = nullptr;
static fn_svm_free retryix_svm_free = nullptr;
static fn_zerocopy_net_init retryix_zerocopy_net_init = nullptr;
static fn_bridge_initialize_universal retryix_bridge_initialize_universal = nullptr;
static fn_atomic_compare_exchange_i32 retryix_atomic_compare_exchange_i32 = nullptr;
static fn_bus_scheduler_init retryix_bus_scheduler_init = nullptr;
static fn_kernel_execute retryix_kernel_execute = nullptr;

bool load_retryix_functions() {
    // 嘗試不同位置載入 DLL
    const char* dll_paths[] = {
        "lib\\retryix.dll",
        "retryix.dll",
        "..\\lib\\retryix.dll"
    };

    for (int i = 0; i < 3; i++) {
        g_dll = LoadLibraryA(dll_paths[i]);
        if (g_dll) {
            printf("✅ 載入 DLL: %s\n", dll_paths[i]);
            break;
        }
    }

    if (!g_dll) {
        printf("❌ 無法載入 retryix.dll\n");
        return false;
    }

    // 載入函數指標
    retryix_initialize = (fn_initialize)GetProcAddress(g_dll, "retryix_initialize");
    retryix_finalize = (fn_finalize)GetProcAddress(g_dll, "retryix_finalize");
    retryix_get_version = (fn_get_version)GetProcAddress(g_dll, "retryix_get_version");
    retryix_strerror = (fn_strerror)GetProcAddress(g_dll, "retryix_strerror");
    retryix_full_system_check = (fn_full_system_check)GetProcAddress(g_dll, "retryix_full_system_check");
    retryix_svm_alloc = (fn_svm_alloc)GetProcAddress(g_dll, "retryix_svm_alloc");
    retryix_svm_free = (fn_svm_free)GetProcAddress(g_dll, "retryix_svm_free");
    retryix_zerocopy_net_init = (fn_zerocopy_net_init)GetProcAddress(g_dll, "retryix_zerocopy_net_init");
    retryix_bridge_initialize_universal = (fn_bridge_initialize_universal)GetProcAddress(g_dll, "retryix_bridge_initialize_universal");
    retryix_atomic_compare_exchange_i32 = (fn_atomic_compare_exchange_i32)GetProcAddress(g_dll, "retryix_atomic_compare_exchange_i32");
    retryix_bus_scheduler_init = (fn_bus_scheduler_init)GetProcAddress(g_dll, "retryix_bus_scheduler_init");
    retryix_kernel_execute = (fn_kernel_execute)GetProcAddress(g_dll, "retryix_kernel_execute");

    return true;
}

void cmd_version() {
    printf("=== 版本資訊 ===\n");

    if (!retryix_get_version) {
        printf("❌ retryix_get_version 函數不可用\n");
        return;
    }

    char version[128] = {0};
    int result = retryix_get_version(version, sizeof(version));

    if (result == 0) {
        printf("版本: %s\n", version);
    } else {
        printf("版本查詢失敗: %d\n", result);
        if (retryix_strerror) {
            printf("錯誤: %s\n", retryix_strerror(result));
        }
    }
}

void cmd_system_check() {
    printf("=== 系統檢查 ===\n");

    if (!retryix_full_system_check) {
        printf("❌ retryix_full_system_check 函數不可用\n");
        return;
    }

    char report[2048] = {0};
    int result = retryix_full_system_check(report, sizeof(report));

    if (result == 0) {
        printf("系統狀態: 正常\n");
        printf("報告: %s\n", report);
    } else {
        printf("系統檢查失敗: %d\n", result);
        if (retryix_strerror) {
            printf("錯誤: %s\n", retryix_strerror(result));
        }
    }
}

void cmd_svm_test() {
    printf("=== SVM 記憶體測試 ===\n");

    if (!retryix_svm_alloc || !retryix_svm_free) {
        printf("❌ SVM 函數不可用\n");
        return;
    }

    size_t sizes[] = {1024, 4096, 16384};

    for (int i = 0; i < 3; i++) {
        printf("測試 %zu bytes... ", sizes[i]);

        void* ptr = retryix_svm_alloc(sizes[i]);
        if (ptr) {
            printf("分配成功 (ptr=%p) ", ptr);

            int free_result = retryix_svm_free(ptr);
            if (free_result == 0) {
                printf("✅ 釋放成功\n");
            } else {
                printf("❌ 釋放失敗: %d\n", free_result);
            }
        } else {
            printf("❌ 分配失敗\n");
        }
    }
}

void cmd_zerocopy_test() {
    printf("=== 零拷貝網路測試 ===\n");

    if (!retryix_zerocopy_net_init) {
        printf("❌ retryix_zerocopy_net_init 函數不可用\n");
        return;
    }

    printf("初始化零拷貝網路... ");
    int result = retryix_zerocopy_net_init();

    if (result == 0) {
        printf("✅ 成功\n");
    } else {
        printf("❌ 失敗: %d\n", result);
        if (retryix_strerror) {
            printf("錯誤: %s\n", retryix_strerror(result));
        }
    }
}

void cmd_bridge_test() {
    printf("=== 硬體橋接測試 ===\n");

    if (!retryix_bridge_initialize_universal) {
        printf("❌ retryix_bridge_initialize_universal 函數不可用\n");
        return;
    }

    printf("初始化通用硬體橋接... ");
    int result = retryix_bridge_initialize_universal();

    if (result == 0) {
        printf("✅ 成功\n");
        printf("支援: NVIDIA CUDA, AMD ROCm, Intel OneAPI\n");
    } else {
        printf("❌ 失敗: %d\n", result);
        if (retryix_strerror) {
            printf("錯誤: %s\n", retryix_strerror(result));
        }
    }
}

void cmd_atomic_test() {
    printf("=== 原子操作測試 ===\n");

    if (!retryix_atomic_compare_exchange_i32) {
        printf("❌ retryix_atomic_compare_exchange_i32 函數不可用\n");
        return;
    }

    volatile int target = 100;
    int expected = 100;
    int desired = 200;
    int previous = 0;

    printf("測試原子比較交換: %d -> %d... ", expected, desired);

    int result = retryix_atomic_compare_exchange_i32(&target, expected, desired, &previous);

    if (result == 0) {
        printf("✅ 成功\n");
        printf("之前值: %d, 當前值: %d\n", previous, (int)target);
    } else {
        printf("❌ 失敗: %d\n", result);
        if (retryix_strerror) {
            printf("錯誤: %s\n", retryix_strerror(result));
        }
    }
}

void cmd_bus_test() {
    printf("=== 匯流排排程器測試 ===\n");

    if (!retryix_bus_scheduler_init) {
        printf("❌ retryix_bus_scheduler_init 函數不可用\n");
        return;
    }

    printf("初始化匯流排排程器... ");
    int result = retryix_bus_scheduler_init();

    if (result == 0) {
        printf("✅ 成功\n");
    } else {
        printf("❌ 失敗: %d\n", result);
        if (retryix_strerror) {
            printf("錯誤: %s\n", retryix_strerror(result));
        }
    }
}

void cmd_kernel_test() {
    printf("=== 核心執行測試 ===\n");

    if (!retryix_kernel_execute) {
        printf("❌ retryix_kernel_execute 函數不可用\n");
        return;
    }

    printf("執行核心... ");
    int result = retryix_kernel_execute();

    if (result == 0) {
        printf("✅ 成功\n");
    } else {
        printf("❌ 失敗: %d\n", result);
        if (retryix_strerror) {
            printf("錯誤: %s\n", retryix_strerror(result));
        }
    }
}

void cmd_comprehensive() {
    printf("=== 綜合測試 ===\n");
    printf("魯班智慧完整驗證\n\n");

    int passed = 0;
    int total = 6;

    // 1. 版本檢查
    printf("1/%d 版本檢查... ", total);
    if (retryix_get_version) {
        char ver[64];
        if (retryix_get_version(ver, sizeof(ver)) == 0) {
            printf("✅ 通過\n");
            passed++;
        } else {
            printf("❌ 失敗\n");
        }
    } else {
        printf("❌ 函數不可用\n");
    }

    // 2. 系統檢查
    printf("2/%d 系統檢查... ", total);
    if (retryix_full_system_check) {
        char report[1024];
        if (retryix_full_system_check(report, sizeof(report)) == 0) {
            printf("✅ 通過\n");
            passed++;
        } else {
            printf("❌ 失敗\n");
        }
    } else {
        printf("❌ 函數不可用\n");
    }

    // 3. SVM 測試
    printf("3/%d SVM 記憶體... ", total);
    if (retryix_svm_alloc && retryix_svm_free) {
        void* ptr = retryix_svm_alloc(1024);
        if (ptr && retryix_svm_free(ptr) == 0) {
            printf("✅ 通過\n");
            passed++;
        } else {
            printf("❌ 失敗\n");
        }
    } else {
        printf("❌ 函數不可用\n");
    }

    // 4. 零拷貝網路
    printf("4/%d 零拷貝網路... ", total);
    if (retryix_zerocopy_net_init) {
        if (retryix_zerocopy_net_init() == 0) {
            printf("✅ 通過\n");
            passed++;
        } else {
            printf("❌ 失敗\n");
        }
    } else {
        printf("❌ 函數不可用\n");
    }

    // 5. 硬體橋接
    printf("5/%d 硬體橋接... ", total);
    if (retryix_bridge_initialize_universal) {
        if (retryix_bridge_initialize_universal() == 0) {
            printf("✅ 通過\n");
            passed++;
        } else {
            printf("❌ 失敗\n");
        }
    } else {
        printf("❌ 函數不可用\n");
    }

    // 6. 原子操作
    printf("6/%d 原子操作... ", total);
    if (retryix_atomic_compare_exchange_i32) {
        volatile int val = 42;
        int prev = 0;
        if (retryix_atomic_compare_exchange_i32(&val, 42, 84, &prev) == 0) {
            printf("✅ 通過\n");
            passed++;
        } else {
            printf("❌ 失敗\n");
        }
    } else {
        printf("❌ 函數不可用\n");
    }

    printf("\n結果: %d/%d 測試通過\n", passed, total);
    if (passed == total) {
        printf("🎯 全部測試通過！RetryIX V7 運作正常！\n");
        printf("🏗️  魯班智慧驗證成功！\n");
    } else {
        printf("⚠️  部分測試失敗，系統可能有問題\n");
    }
}

void cmd_help() {
    printf("=== RetryIX V7 簡化 CLI 指令 ===\n");
    printf("魯班智慧應用 - HBM 突破技術\n\n");
    printf("可用指令:\n");
    printf("  version       - 顯示版本資訊\n");
    printf("  system-check  - 執行系統檢查\n");
    printf("  svm-test      - 測試 SVM 記憶體\n");
    printf("  zerocopy-test - 測試零拷貝網路\n");
    printf("  bridge-test   - 測試硬體橋接\n");
    printf("  atomic-test   - 測試原子操作\n");
    printf("  bus-test      - 測試匯流排排程器\n");
    printf("  kernel-test   - 測試核心執行\n");
    printf("  comprehensive - 執行綜合測試\n");
    printf("  help          - 顯示此說明\n");
    printf("  quit          - 結束程式\n");
}

int main() {
    printf("=== RetryIX V7 簡化 CLI ===\n");
    printf("HBM 突破技術與魯班智慧\n");
    printf("版本: 7.0.0-RC1-LuBan\n\n");

    // 載入 DLL 和函數
    if (!load_retryix_functions()) {
        return 1;
    }

    // 初始化 RetryIX
    bool initialized = false;
    if (retryix_initialize) {
        int result = retryix_initialize();
        if (result == 0) {
            printf("✅ RetryIX V7 初始化成功\n");
            initialized = true;
        } else {
            printf("❌ RetryIX 初始化失敗: %d\n", result);
        }
    } else {
        printf("❌ retryix_initialize 函數不可用\n");
    }

    cmd_help();

    char line[256];
    while (true) {
        printf("\nretryix> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        // 移除換行
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }

        if (len == 0) continue;

        // 處理指令
        if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            break;
        } else if (strcmp(line, "version") == 0) {
            cmd_version();
        } else if (strcmp(line, "system-check") == 0) {
            cmd_system_check();
        } else if (strcmp(line, "svm-test") == 0) {
            cmd_svm_test();
        } else if (strcmp(line, "zerocopy-test") == 0) {
            cmd_zerocopy_test();
        } else if (strcmp(line, "bridge-test") == 0) {
            cmd_bridge_test();
        } else if (strcmp(line, "atomic-test") == 0) {
            cmd_atomic_test();
        } else if (strcmp(line, "bus-test") == 0) {
            cmd_bus_test();
        } else if (strcmp(line, "kernel-test") == 0) {
            cmd_kernel_test();
        } else if (strcmp(line, "comprehensive") == 0) {
            cmd_comprehensive();
        } else if (strcmp(line, "help") == 0) {
            cmd_help();
        } else {
            printf("未知指令: %s\n", line);
            printf("輸入 'help' 查看可用指令\n");
        }
    }

    // 清理
    if (initialized && retryix_finalize) {
        int result = retryix_finalize();
        if (result == 0) {
            printf("✅ RetryIX V7 結束成功\n");
        } else {
            printf("⚠️  RetryIX 結束警告: %d\n", result);
        }
    }

    if (g_dll) {
        FreeLibrary(g_dll);
    }

    printf("🏗️  魯班智慧 CLI 完成！\n");
    return 0;
}