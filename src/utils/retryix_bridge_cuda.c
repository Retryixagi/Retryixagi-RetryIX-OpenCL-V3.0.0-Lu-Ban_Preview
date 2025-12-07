// retryix_bridge_cuda.c
// CUDA 平台橋接工具
#include "retryix_bridge_cuda.h"
#include <string.h>


// 動態載入工具
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#define DLOPEN(x)   LoadLibraryA(x)
#define DLSYM(h, n) GetProcAddress((HMODULE)h, n)
#define DLCLOSE(h)  FreeLibrary((HMODULE)h)
#else
#include <dlfcn.h>
#define DLOPEN(x)   dlopen(x, RTLD_NOW)
#define DLSYM(h, n) dlsym(h, n)
#define DLCLOSE(h)  dlclose(h)
#endif

#ifdef RETRYIX_HAS_CUDA
#include <cuda_runtime.h>
#endif

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_cuda_platforms(
    retryix_platform_t* platforms,
    int max_platforms,
    int* platform_count
) {
    if (!platforms || !platform_count || max_platforms <= 0)
        return RETRYIX_ERROR_NULL_PTR;
#ifdef RETRYIX_HAS_CUDA

    // 動態載入 cudart64_*.dll (Windows) 或 libcudart.so (Linux)
    void* hCudart = NULL;
#ifdef _WIN32
    const char* dlls[] = {
        "cudart64_120.dll", "cudart64_110.dll", "cudart64_101.dll", "cudart64_100.dll",
        "cudart64_92.dll", "cudart64_91.dll", "cudart64_90.dll", "cudart64_80.dll"
    };
    for (int i = 0; i < sizeof(dlls)/sizeof(dlls[0]); ++i) {
        hCudart = DLOPEN(dlls[i]);
        if (hCudart) break;
    }
    // Driver API (nvcuda.dll) 可選擇性支援
    if (!hCudart) hCudart = DLOPEN("nvcuda.dll");
#else
    hCudart = DLOPEN("libcudart.so");
    if (!hCudart) hCudart = DLOPEN("libcudart.so.11.0");
    if (!hCudart) hCudart = DLOPEN("libcudart.so.10.1");
    if (!hCudart) hCudart = DLOPEN("libcudart.so.10.0");
#endif
    if (!hCudart) {
        *platform_count = 0;
        return RETRYIX_ERROR_NO_PLATFORM;
    }

    typedef cudaError_t (*PFN_cudaGetDeviceCount)(int*);
    typedef cudaError_t (*PFN_cudaGetDeviceProperties)(void*, int);
    PFN_cudaGetDeviceCount pCount = (PFN_cudaGetDeviceCount)DLSYM(hCudart, "cudaGetDeviceCount");
    PFN_cudaGetDeviceProperties pProps = (PFN_cudaGetDeviceProperties)DLSYM(hCudart, "cudaGetDeviceProperties");
    if (!pCount || !pProps) {
        *platform_count = 0;
    DLCLOSE(hCudart);
    return RETRYIX_ERROR_NO_PLATFORM;
    }

    int cuda_count = 0;
    cudaError_t err = pCount(&cuda_count);
    if (err != 0 || cuda_count <= 0) {
        *platform_count = 0;
    DLCLOSE(hCudart);
    return RETRYIX_ERROR_NO_PLATFORM;
    }
    int n = (cuda_count > max_platforms) ? max_platforms : cuda_count;
    int valid_count = 0;
    for (int i = 0; i < n; ++i) {
        retryix_platform_t* plat = &platforms[valid_count];
        memset(plat, 0, sizeof(retryix_platform_t));
        cudaDeviceProp prop;
        cudaError_t pe = pProps(&prop, i);
        if (pe != 0) continue;

        int maj = prop.major, min = prop.minor;
        char cc_series[64] = "";
        if (maj == 8 && min == 0) strcpy(cc_series, "Ampere (A100/H100)");
        else if (maj == 8 && min == 6) strcpy(cc_series, "Ada (4090/4080/4070)");
        else if (maj == 7 && min == 0) strcpy(cc_series, "Volta (V100)");
        else if (maj == 7 && min == 5) strcpy(cc_series, "Turing (2080/2070/2060)");
        else if (maj == 6) strcpy(cc_series, "Pascal (1080/1070/1060)");
        else if (maj == 5) strcpy(cc_series, "Maxwell (980/970)");
        else if (maj == 3) strcpy(cc_series, "Kepler (780/770)");
        else strcpy(cc_series, "NVIDIA GPU");

        const char* model = prop.name;
        char ui_series[64] = "";
        if (strstr(model, "Ti")) {
            if (strstr(model, "1000") || strstr(model, "1080") || strstr(model, "1070") || strstr(model, "1060")) strcpy(ui_series, "NVIDIA 1000 Ti Series");
            else if (strstr(model, "2000") || strstr(model, "2080") || strstr(model, "2070") || strstr(model, "2060")) strcpy(ui_series, "NVIDIA 2000 Ti Series");
            else if (strstr(model, "3000") || strstr(model, "3080") || strstr(model, "3070") || strstr(model, "3060")) strcpy(ui_series, "NVIDIA 3000 Ti Series");
            else if (strstr(model, "4000") || strstr(model, "4090") || strstr(model, "4080") || strstr(model, "4070") || strstr(model, "4060")) strcpy(ui_series, "NVIDIA 4000 Ti Series");
            else if (strstr(model, "5000") || strstr(model, "5090") || strstr(model, "5080") || strstr(model, "5070") || strstr(model, "5060")) strcpy(ui_series, "NVIDIA 5000 Ti Series");
            else strcpy(ui_series, "NVIDIA Ti Series");
        } else if (strstr(model, "Super")) {
            if (strstr(model, "1000") || strstr(model, "1080") || strstr(model, "1070") || strstr(model, "1060")) strcpy(ui_series, "NVIDIA 1000 Super Series");
            else if (strstr(model, "2000") || strstr(model, "2080") || strstr(model, "2070") || strstr(model, "2060")) strcpy(ui_series, "NVIDIA 2000 Super Series");
            else if (strstr(model, "3000") || strstr(model, "3080") || strstr(model, "3070") || strstr(model, "3060")) strcpy(ui_series, "NVIDIA 3000 Super Series");
            else if (strstr(model, "4000") || strstr(model, "4090") || strstr(model, "4080") || strstr(model, "4070") || strstr(model, "4060")) strcpy(ui_series, "NVIDIA 4000 Super Series");
            else if (strstr(model, "5000") || strstr(model, "5090") || strstr(model, "5080") || strstr(model, "5070") || strstr(model, "5060")) strcpy(ui_series, "NVIDIA 5000 Super Series");
            else strcpy(ui_series, "NVIDIA Super Series");
        } else if (strstr(model, "1000") || strstr(model, "1080") || strstr(model, "1070") || strstr(model, "1060")) strcpy(ui_series, "NVIDIA 1000 Series");
        else if (strstr(model, "2000") || strstr(model, "2080") || strstr(model, "2070") || strstr(model, "2060")) strcpy(ui_series, "NVIDIA 2000 Series");
        else if (strstr(model, "3000") || strstr(model, "3080") || strstr(model, "3070") || strstr(model, "3060")) strcpy(ui_series, "NVIDIA 3000 Series");
        else if (strstr(model, "4000") || strstr(model, "4090") || strstr(model, "4080") || strstr(model, "4070") || strstr(model, "4060")) strcpy(ui_series, "NVIDIA 4000 Series");
        else if (strstr(model, "5000") || strstr(model, "5090") || strstr(model, "5080") || strstr(model, "5070") || strstr(model, "5060")) strcpy(ui_series, "NVIDIA 5000 Series");
        else if (strstr(model, "A100")) strcpy(ui_series, "NVIDIA A100");
        else if (strstr(model, "H100")) strcpy(ui_series, "NVIDIA H100");
        else if (strstr(model, "J100")) strcpy(ui_series, "NVIDIA J100");
        else strcpy(ui_series, "NVIDIA GPU");

        snprintf(plat->name, sizeof(plat->name), "%s (%s, %s)", model, cc_series, ui_series);
        snprintf(plat->vendor, sizeof(plat->vendor), "NVIDIA");
        snprintf(plat->version, sizeof(plat->version), "CUDA CC %d.%d", maj, min);
        snprintf(plat->profile, sizeof(plat->profile), "CUDA");
        plat->device_count = 1;
        ++valid_count;
    }
    *platform_count = valid_count;
    DLCLOSE(hCudart);
    return RETRYIX_SUCCESS;
#else
    *platform_count = 0;
    return RETRYIX_ERROR_NO_PLATFORM;
#endif
}
