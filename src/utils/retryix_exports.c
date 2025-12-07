// retryix_exports.c
// Thin exported wrappers for Python/ctypes without touching original files.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "retryix_opencl_compat.h"
#include "retryix.h"

// 若 retryix.h 未定義 RetryIXDevice,則補充定義
#ifndef RETRYIXDEVICE_DEFINED
#define RETRYIXDEVICE_DEFINED
typedef struct {
    char name[256];
    cl_device_type type;
    cl_ulong global_mem_size;
    char opencl_version[128];
    cl_device_id id;
} RetryIXDevice;
#endif

// 若 retryix.h 未定義 RetryIXDevice，則補充定義
#ifndef RETRYIXDEVICE_DEFINED
#define RETRYIXDEVICE_DEFINED
typedef struct {
    char name[256];
    cl_device_type type;
    cl_ulong global_mem_size;
    char opencl_version[128];
    cl_device_id id;
} RetryIXDevice;
#endif

#ifdef _WIN32
  #define RIX_API __declspec(dllexport)
  #define RIX_CDECL __cdecl
#else
  #define RIX_API
  #define RIX_CDECL
#endif


// ---- Custom SDK Version & selftest ----
RIX_API int RIX_CDECL retryix_sdk_get_version(int* major, int* minor, int* patch) {
    if (!major || !minor || !patch) return -1;
    *major = 2; *minor = 0; *patch = 0;
    return 0;
}

RIX_API int RIX_CDECL retryix_sdk_selftest(void) {
    return 0;
}


// ---- Device count via custom SDK ----
RIX_API int RIX_CDECL retryix_sdk_device_count(void) {
    // 建議改用主 SDK API，如 retryix_discover_all_devices
    return 0; // stub, 可根據實際需求補齊
}


// ---- Enumerate devices to JSON (in-memory) ----
static int sdk_read_all(const char* path, char** out_buf, size_t* out_len) {
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return -1; }
    rewind(f);
    char* mem = (char*)malloc((size_t)sz + 1);
    if (!mem) { fclose(f); return -1; }
    size_t rd = fread(mem, 1, (size_t)sz, f);
    fclose(f);
    mem[rd] = '\0';
    *out_buf = mem;
    *out_len = rd;
    return 0;
}

RIX_API int RIX_CDECL retryix_sdk_enumerate_devices_json(char* buf, unsigned long cap) {
    const char* tmp_path = "retryix_device_report_tmp.json";
    // 建議改用主 SDK API，如 retryix_export_all_devices_json
    char* file_buf = NULL;
    size_t file_len = 0;
    if (sdk_read_all(tmp_path, &file_buf, &file_len) != 0) {
        return -6;
    }
    if (buf == NULL || cap == 0) {
        int need = (int)file_len;
        free(file_buf);
        return need;
    }
    if (cap <= file_len) {
        free(file_buf);
        return -5;
    }
    memcpy(buf, file_buf, file_len);
    buf[file_len] = '\0';
    free(file_buf);
    return (int)file_len;
}


// ---- Optional: 最後錯誤字串 ----
RIX_API int RIX_CDECL retryix_sdk_last_error(char* buf, unsigned long cap) {
    const char* s = "";
    size_t n = strlen(s);
    if (!buf || cap == 0) return (int)n;
    if (cap <= n) return -5;
    memcpy(buf, s, n + 1);
    return (int)n;
}
