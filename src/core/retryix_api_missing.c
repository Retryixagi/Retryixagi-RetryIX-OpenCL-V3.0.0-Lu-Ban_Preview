/**
 * @file retryix_api_missing.c
 * @brief RetryIX v3.0.0 - Restore missing high-level API functions
 * @version 3.0.0-LuBan
 */

#include "retryix.h"
#include "retryix_core.h"
#include "retryix_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ===== Global State ===== */
static bool g_retryix_initialized = false;
static char g_last_error[256] = {0};
static retryix_device_t* g_devices = NULL;
static size_t g_device_count = 0;

static void set_last_error(const char* msg) {
    if (msg) {
        snprintf(g_last_error, sizeof(g_last_error), "%s", msg);
    }
}

/* ===== Version Info ===== */
RETRYIX_API const char* RETRYIX_CALL retryix_get_version(void) {
    return "3.0.0-LuBan";
}

RETRYIX_API const char* RETRYIX_CALL retryix_sdk_get_version(void) {
    return "SDK v3.0.0-LuBan (Simulation Mode)";
}
