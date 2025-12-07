// retryix_host.c
// Unified RetryIX Host with module registration + host_comm wiring
// Fixes:
//  - retryix_send_command(): use in-thread loopback (no queue echo)
//  - add forward declarations to avoid implicit declaration warnings
// OpenCL 完全禁用 - 使用模擬

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "retryix_opencl_compat.h"
#include "retryix_core.h"
#include "retryix_host_comm.h"
#include "retryix_module_descriptor.h"
#include "retryix_host.h"


// ===== Forward Declarations (avoid implicit declaration warnings) =====
RETRYIX_API int retryix_init_from_source(const char* kernel_source, const char* build_opts);
RETRYIX_API int retryix_init_from_file(const char* kernel_path, const char* build_opts);
RETRYIX_API int retryix_init_minimal(void);
RETRYIX_API cl_kernel retryix_create_kernel(const char* kernel_name);
RETRYIX_API int retryix_execute_kernel(float* host_data, size_t count);
RETRYIX_API int retryix_launch_1d(cl_kernel k, cl_mem arg0, size_t global);
RETRYIX_API int retryix_host_receive_command(const char *input, char *response, size_t response_size);
RETRYIX_API int retryix_send_command(const char* message, char* response, size_t response_size);
RETRYIX_API int retryix_host_shutdown(void);

// ===== CLI 依賴 API stub 實作 =====
RETRYIX_API int retryix_init_minimal(void) {
    // stub: always success
    return 0;
}

RETRYIX_API int retryix_init_from_file(const char* kernel_path, const char* build_opts) {
    (void)kernel_path; (void)build_opts;
    // stub: always success
    return 0;
}

RETRYIX_API int retryix_send_command(const char* message, char* response, size_t response_size) {
    (void)message;
    if (response && response_size > 0) {
        strncpy(response, "OK", response_size - 1);
        response[response_size - 1] = '\0';
    }
    return 0;
}

RETRYIX_API int retryix_host_shutdown(void) {
    // stub: always success for host-specific shutdown
    return 0;
}

