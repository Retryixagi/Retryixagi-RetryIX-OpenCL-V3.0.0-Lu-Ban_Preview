// retryix_bridge_rocm.c
// ROCm/AMD GPU 平台橋接工具
#include "retryix_bridge_rocm.h"
#include <string.h>
#include <stdio.h>

// 動態載入 HIP/ROCm 工具
#ifdef _WIN32
#include <windows.h>
#define DLOPEN(x)   LoadLibraryA(x)
#define DLSYM(h, n) GetProcAddress((HMODULE)h, n)
#define DLCLOSE(h)  FreeLibrary((HMODULE)h)
#define RTLD_HANDLE HMODULE
#else
#include <dlfcn.h>
#define DLOPEN(x)   dlopen(x, RTLD_NOW)
#define DLSYM(h, n) dlsym(h, n)
#define DLCLOSE(h)  dlclose(h)
#define RTLD_HANDLE void*
#endif

// ---- Minimal HIP typedefs（用函式而非 struct，避免 header 相依） ----
typedef int hipError_t;
typedef int hipDevice_t;

typedef hipError_t (*PFN_hipInit)(unsigned int flags);
typedef hipError_t (*PFN_hipGetDeviceCount)(int* count);
typedef hipError_t (*PFN_hipGetDevice)(int* device);
typedef hipError_t (*PFN_hipDeviceGetName)(char* name, int len, int device);
typedef hipError_t (*PFN_hipDeviceComputeCapability)(int* major, int* minor, int device);
typedef hipError_t (*PFN_hipDriverGetVersion)(int* driverVersion);
typedef hipError_t (*PFN_hipRuntimeGetVersion)(int* runtimeVersion);

static inline int hip_ok(hipError_t e) { return e==0; }

static RTLD_HANDLE rix_open_hip(void) {
#ifdef _WIN32
    RTLD_HANDLE h = DLOPEN("amdhip64.dll");      // Windows ROCm/HIP
#else
    RTLD_HANDLE h = DLOPEN("libamdhip64.so");    // Linux ROCm/HIP (通用名)
    if (!h) h = DLOPEN("libamdhip64.so.5");      // 版本後綴
    if (!h) h = DLOPEN("libhip_hcc.so");         // 最後遺產 fallback（舊版）
#endif
    return h;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_rocm_platforms(
    retryix_platform_t* platforms, int max_platforms, int* platform_count)
{
    if (!platforms || !platform_count || max_platforms <= 0)
        return RETRYIX_ERROR_NULL_PTR;

    RTLD_HANDLE h = rix_open_hip();
    if (!h) { *platform_count = 0; return RETRYIX_ERROR_NO_PLATFORM; }

    // 綁定必要函式
    PFN_hipInit                 p_hipInit                 = (PFN_hipInit)(void*)                DLSYM(h, "hipInit");
    PFN_hipGetDeviceCount       p_hipGetDeviceCount       = (PFN_hipGetDeviceCount)(void*)      DLSYM(h, "hipGetDeviceCount");
    PFN_hipDeviceGetName        p_hipDeviceGetName        = (PFN_hipDeviceGetName)(void*)       DLSYM(h, "hipDeviceGetName");
    PFN_hipDeviceComputeCapability p_hipDeviceComputeCapability = (PFN_hipDeviceComputeCapability)(void*) DLSYM(h, "hipDeviceComputeCapability");
    PFN_hipDriverGetVersion     p_hipDriverGetVersion     = (PFN_hipDriverGetVersion)(void*)    DLSYM(h, "hipDriverGetVersion");
    PFN_hipRuntimeGetVersion    p_hipRuntimeGetVersion    = (PFN_hipRuntimeGetVersion)(void*)   DLSYM(h, "hipRuntimeGetVersion");

    if (!p_hipGetDeviceCount || !p_hipDeviceGetName || !p_hipDeviceComputeCapability) {
        DLCLOSE(h);
        *platform_count = 0;
        return RETRYIX_ERROR_NO_PLATFORM;
    }

    if (p_hipInit) {
        // 某些環境需要顯式 init；若沒有此函式也沒關係
        (void)p_hipInit(0);
    }

    int count = 0;
    if (!hip_ok(p_hipGetDeviceCount(&count)) || count <= 0) {
        DLCLOSE(h);
        *platform_count = 0;
        return RETRYIX_ERROR_NO_PLATFORM;
    }

    int n = (count > max_platforms) ? max_platforms : count;
    int out = 0;
    int drv_ver = 0, rt_ver = 0;
    if (p_hipDriverGetVersion)  (void)p_hipDriverGetVersion(&drv_ver);
    if (p_hipRuntimeGetVersion) (void)p_hipRuntimeGetVersion(&rt_ver);

    for (int i = 0; i < n; ++i) {
        char name[256] = {0};
        int maj=0, min=0;

        if (!hip_ok(p_hipDeviceGetName(name, sizeof(name), i))) continue;
        if (!hip_ok(p_hipDeviceComputeCapability(&maj, &min, i))) { maj=0; min=0; }

        retryix_platform_t* plat = &platforms[out];
        memset(plat, 0, sizeof(*plat));

        // 主要識別
        snprintf(plat->vendor,  sizeof(plat->vendor),  "AMD");
        snprintf(plat->name,    sizeof(plat->name),    "%s", name);
        snprintf(plat->version, sizeof(plat->version), "HIP CC %d.%d (drv %d, rt %d)", maj, min, drv_ver, rt_ver);
        snprintf(plat->profile, sizeof(plat->profile), "ROCm/HIP");
        plat->device_count = 1;

        // 可選：根據名字/CC 粗分系列（避免大量 hardcode）
        // e.g., gfx103* → RDNA2, gfx110* → RDNA3（需要另行查詢 gcnArchName，這裡先略）
        ++out;
    }

    *platform_count = out;
    DLCLOSE(h);
    return (out > 0) ? RETRYIX_SUCCESS : RETRYIX_ERROR_NO_PLATFORM;
}
