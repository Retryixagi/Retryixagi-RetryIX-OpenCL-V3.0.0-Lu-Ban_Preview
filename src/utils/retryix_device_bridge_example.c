/*
 * retryix_device_bridge_example.c
 * RetryIX 統一設備橋接系統 C 語言使用示例
 */
#include "retryix_unified_device_bridge.h"
#include "retryix_compute_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ===== 示例 1: 基本設備檢測和初始化 =====
void demo_basic_initialization() {
    printf("\n=== Demo 1: Basic Device Detection ===\n");
    if (retryix_initialize_device_bridge(RETRYIX_BACKEND_AUTO) != 0) {
        printf("Failed to initialize device bridge!\n");
        return;
    }
    int kinds[8];
    int count = retryix_get_available_backends(kinds, 8);
    printf("Found %d available backends:\n", count);
    for (int i = 0; i < count; ++i) {
        retryix_backend_t* backend = retryix_get_backend(kinds[i]);
        if (backend) {
            const retryix_device_info_t* device = backend->device(backend);
            printf("  - %s: %s (%s)\n", retryix_backend_kind_to_string(kinds[i]), device->name, device->vendor);
        }
    }
    retryix_backend_t* primary = retryix_get_primary_backend();
    if (primary) {
        printf("\nPrimary backend: %s\n", retryix_backend_kind_to_string(primary->kind(primary)));
        printf("Device: %s\n", primary->device(primary)->name);
    }
}

// 其他示例可依 C 語法改寫，包含向量加法、圖像濾鏡、性能比較、互操作、設備管理等

int main() {
    printf("RetryIX Unified Device Bridge System Demo (C version)\n");
    printf("==========================================\n");
    demo_basic_initialization();
    // 其他 demo_xxx() 可依需求補充
    retryix_shutdown_device_bridge();
    printf("Demo completed successfully!\n");
    return 0;
}
