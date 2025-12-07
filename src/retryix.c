
#include <stdio.h>
// retryix.c - RetryIX Main Implementation
#include "retryix.h"
#include "retryix_svm.h"
#include "retryix_export.h"
#include <string.h>

static retryix_system_state_t g_state = {0};

#ifdef RETRYIX_DEBUG
RETRYIX_API void RETRYIX_CALL retryix_debug_print_system_info(void) {
    printf("[RetryIX] Debug System Info\n");
    printf("Initialized: %d\n", g_state.is_initialized);
    printf("Device count: %d\n", g_state.device_count);
    printf("Best device: %s\n", g_state.best_device.name);
}
#endif

// 主程式入口，避免 WinMain 錯誤
int main(int argc, char** argv) {
    printf("RetryIX tool started.\n");
    // 可根據需求加入更多 CLI 邏輯
    return 0;
}


// This file intentionally keeps only minimal system-state storage and the
// lightweight getter so that `src/core/retryix_api.c` can provide the
// canonical public API implementations. Having a single definition of the
// public APIs prevents multiple-definition linker errors when building the
// combined static/dynamic artifacts.
