#define RETRYIX_BUILD_DLL
#ifndef __ANDROID__
    typedef void* EGLContext;
    typedef void* EGLSurface;
    #define EGL_NO_CONTEXT  ((EGLContext)0)
    #define EGL_NO_SURFACE  ((EGLSurface)0)
    #define EGL_DEFAULT_DISPLAY ((void*)0)
    #define EGL_NO_DISPLAY     ((void*)0)
#endif

// retryix_bridge_android.c - Android GPU驅動橋接實現
#include "retryix_bridge_android.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __ANDROID__
#include <dlfcn.h>
#include <unistd.h>
#include <sys/system_properties.h>
#else
// 非Android平台的占位符
#define F_OK 0
static inline int access(const char* path, int mode) { (void)path; (void)mode; return -1; }
static inline void* dlopen(const char* filename, int flag) { (void)filename; (void)flag; return NULL; }
static inline void* dlsym(void* handle, const char* symbol) { (void)handle; (void)symbol; return NULL; }
static inline int dlclose(void* handle) { (void)handle; return 0; }
static inline int __system_property_get(const char* name, char* value) { (void)name; (void)value; return 0; }
// EGL占位符函數
static inline void* eglGetDisplay(void* display_id) { (void)display_id; return (void*)0; }
static inline int eglInitialize(void* display, void* major, void* minor) { (void)display; (void)major; (void)minor; return 0; }
static inline int eglChooseConfig(void* display, const int* attrib_list, void* configs, int config_size, int* num_config) {
    (void)display; (void)attrib_list; (void)configs; (void)config_size; (void)num_config; return 0; }
static inline void* eglCreateContext(void* display, void* config, void* share_context, const int* attrib_list) {
    (void)display; (void)config; (void)share_context; (void)attrib_list; return EGL_NO_CONTEXT; }
static inline void* eglCreatePbufferSurface(void* display, void* config, const int* attrib_list) {
    (void)display; (void)config; (void)attrib_list; return EGL_NO_SURFACE; }
static inline int eglMakeCurrent(void* display, void* draw, void* read, void* context) {
    (void)display; (void)draw; (void)read; (void)context; return 0; }
static inline int eglDestroyContext(void* display, void* context) { (void)display; (void)context; return 0; }
static inline int eglDestroySurface(void* display, void* surface) { (void)display; (void)surface; return 0; }
static inline int eglTerminate(void* display) { (void)display; return 0; }
static inline const char* glGetString(unsigned int name) { (void)name; return "Non-Android Platform"; }
#endif
#ifdef __ANDROID__
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#else
// 非Android平台的占位符定義
#define EGL_DEFAULT_DISPLAY ((void*)0)
#define EGL_NO_DISPLAY ((void*)0)
#define EGL_NO_CONTEXT ((void*)0)
#define EGL_NO_SURFACE ((void*)0)
#define RTLD_LAZY 1
#define RTLD_NOW 2
#define EGL_SURFACE_TYPE 0x3033
#define EGL_PBUFFER_BIT 0x0001
#define EGL_RENDERABLE_TYPE 0x3040
#define EGL_OPENGL_ES2_BIT 0x0004
#define EGL_NONE 0x3038
#define EGL_CONTEXT_CLIENT_VERSION 0x3098
#define EGL_WIDTH 0x3057
#define EGL_HEIGHT 0x3056
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03
typedef void* EGLDisplay;
typedef void* EGLConfig;
typedef void* EGLContext;
typedef void* EGLSurface;
typedef int EGLint;
#endif

// ===== Android系統屬性獲取 =====
static int android_get_prop(const char* key, char* value, size_t size) {
    if (!key || !value || size == 0) return -1;

    // 使用__system_property_get (Android專用)
    return __system_property_get(key, value);
}

// ===== GPU類型檢測 =====
static retryix_android_gpu_type_t detect_gpu_from_renderer(const char* renderer) {
    if (!renderer) return RETRYIX_ANDROID_GPU_UNKNOWN;

    if (strstr(renderer, "Adreno") != NULL) {
        return RETRYIX_ANDROID_GPU_ADRENO;
    } else if (strstr(renderer, "Mali") != NULL) {
        return RETRYIX_ANDROID_GPU_MALI;
    } else if (strstr(renderer, "PowerVR") != NULL) {
        return RETRYIX_ANDROID_GPU_POWERVR;
    } else if (strstr(renderer, "Tegra") != NULL) {
        return RETRYIX_ANDROID_GPU_TEGRA;
    } else if (strstr(renderer, "Intel") != NULL) {
        return RETRYIX_ANDROID_GPU_INTEL;
    } else if (strstr(renderer, "Vivante") != NULL) {
        return RETRYIX_ANDROID_GPU_VIVANTE;
    }

    return RETRYIX_ANDROID_GPU_UNKNOWN;
}

// ===== 實現Android平台API =====

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_detect_gpu_type(
    retryix_android_gpu_type_t* gpu_type,
    retryix_android_device_info_t* device_info
) {
    if (!gpu_type || !device_info) return RETRYIX_ERROR_NULL_PTR;

    // 初始化EGL來獲取GPU信息
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        *gpu_type = RETRYIX_ANDROID_GPU_UNKNOWN;
        return RETRYIX_ERROR_NO_DEVICE;
    }

    if (!eglInitialize(display, NULL, NULL)) {
        *gpu_type = RETRYIX_ANDROID_GPU_UNKNOWN;
        return RETRYIX_ERROR_NO_DEVICE;
    }

    // 創建簡單的EGL上下文來查詢GPU信息
    EGLConfig config;
    EGLint num_configs;
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    if (!eglChooseConfig(display, config_attribs, &config, 1, &num_configs) || num_configs == 0) {
        eglTerminate(display);
        return RETRYIX_ERROR_NO_DEVICE;
    }

    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribs);
    if (context == EGL_NO_CONTEXT) {
        eglTerminate(display);
        return RETRYIX_ERROR_NO_DEVICE;
    }

    // 創建虛擬表面
    EGLint pbuffer_attribs[] = {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_NONE
    };
    EGLSurface surface = eglCreatePbufferSurface(display, config, pbuffer_attribs);

    if (!eglMakeCurrent(display, surface, surface, context)) {
        eglDestroyContext(display, context);
        eglTerminate(display);
        return RETRYIX_ERROR_NO_DEVICE;
    }

    // 獲取GPU信息
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* version = (const char*)glGetString(GL_VERSION);
    const char* extensions = (const char*)glGetString(GL_EXTENSIONS);

    // 填充設備信息
    if (vendor) strncpy(device_info->vendor, vendor, sizeof(device_info->vendor) - 1);
    if (renderer) strncpy(device_info->renderer, renderer, sizeof(device_info->renderer) - 1);
    if (version) strncpy(device_info->version, version, sizeof(device_info->version) - 1);
    if (extensions) strncpy(device_info->extensions, extensions, sizeof(device_info->extensions) - 1);

    // 檢測GPU類型
    *gpu_type = detect_gpu_from_renderer(renderer);
    device_info->gpu_type = *gpu_type;

    // 檢測計算支援
    device_info->supports_gles_compute = (extensions && strstr(extensions, "GL_ARB_compute_shader"));

    // 清理EGL
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);

    return RETRYIX_SUCCESS;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_get_system_property(
    const char* key,
    char* value,
    size_t value_size
) {
    if (!key || !value || value_size == 0) return RETRYIX_ERROR_NULL_PTR;

    int result = android_get_prop(key, value, value_size);
    return (result > 0) ? RETRYIX_SUCCESS : RETRYIX_ERROR_UNKNOWN;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_scan_opencl_drivers(
    retryix_android_opencl_driver_t* drivers,
    int max_drivers,
    int* driver_count
) {
    if (!drivers || !driver_count || max_drivers <= 0) return RETRYIX_ERROR_NULL_PTR;

    *driver_count = 0;

    // 常見的Android OpenCL驅動路徑
    const char* opencl_libs[] = {
        RETRYIX_ANDROID_OPENCL_ADRENO_LIB,
        RETRYIX_ANDROID_OPENCL_ADRENO_LIB32,
        RETRYIX_ANDROID_OPENCL_MALI_LIB,
        RETRYIX_ANDROID_OPENCL_MALI_LIB32,
        RETRYIX_ANDROID_OPENCL_POWERVR_LIB,
        RETRYIX_ANDROID_OPENCL_POWERVR_LIB32,
        "/system/lib64/libOpenCL.so",
        "/system/lib/libOpenCL.so",
        "/vendor/lib64/libOpenCL.so",
        "/vendor/lib/libOpenCL.so"
    };

    int num_libs = sizeof(opencl_libs) / sizeof(opencl_libs[0]);

    for (int i = 0; i < num_libs && *driver_count < max_drivers; i++) {
        if (access(opencl_libs[i], F_OK) == 0) {
            retryix_android_opencl_driver_t* driver = &drivers[*driver_count];

            strncpy(driver->lib_path, opencl_libs[i], sizeof(driver->lib_path) - 1);
            driver->lib_path[sizeof(driver->lib_path) - 1] = '\0';

            // 嘗試載入庫來驗證
            void* handle = dlopen(opencl_libs[i], RTLD_LAZY);
            if (handle != NULL) {
                driver->is_available = true;
                driver->handle = handle;

                // 檢測供應商
                if (strstr(opencl_libs[i], "adreno")) {
                    strncpy(driver->vendor, "Qualcomm", sizeof(driver->vendor) - 1);
                } else if (strstr(opencl_libs[i], "mali")) {
                    strncpy(driver->vendor, "ARM", sizeof(driver->vendor) - 1);
                } else if (strstr(opencl_libs[i], "powervr")) {
                    strncpy(driver->vendor, "Imagination", sizeof(driver->vendor) - 1);
                } else {
                    strncpy(driver->vendor, "Unknown", sizeof(driver->vendor) - 1);
                }

                (*driver_count)++;
                dlclose(handle); // 暫時關閉，只是檢測
                driver->handle = NULL;
            } else {
                driver->is_available = false;
            }
        }
    }

    return (*driver_count > 0) ? RETRYIX_SUCCESS : RETRYIX_ERROR_NO_DEVICE;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_load_opencl_driver(
    const char* lib_path,
    void** driver_handle
) {
    if (!lib_path || !driver_handle) return RETRYIX_ERROR_NULL_PTR;

    void* handle = dlopen(lib_path, RTLD_NOW);
    if (!handle) {
        return RETRYIX_ERROR_OPENCL;
    }

    // 檢查基本OpenCL函數是否存在
    if (!dlsym(handle, "clGetPlatformIDs")) {
        dlclose(handle);
        return RETRYIX_ERROR_OPENCL;
    }

    *driver_handle = handle;
    return RETRYIX_SUCCESS;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_android_platforms(
    retryix_platform_t* platforms,
    int max_platforms,
    int* platform_count
) {
    if (!platforms || !platform_count || max_platforms <= 0) return RETRYIX_ERROR_NULL_PTR;

    *platform_count = 0;

    // 掃描OpenCL驅動
    retryix_android_opencl_driver_t drivers[RETRYIX_ANDROID_MAX_GPU_DRIVERS];
    int driver_count = 0;

    retryix_result_t result = retryix_android_scan_opencl_drivers(drivers, RETRYIX_ANDROID_MAX_GPU_DRIVERS, &driver_count);
    if (result != RETRYIX_SUCCESS) {
        return result;
    }

    // 獲取Android設備信息
    retryix_android_device_info_t device_info = {0};
    retryix_android_gpu_type_t gpu_type;

    retryix_android_detect_gpu_type(&gpu_type, &device_info);

    // 獲取Android系統信息
    char prop_value[RETRYIX_ANDROID_PROP_VALUE_MAX];

    if (android_get_prop("ro.build.version.sdk", prop_value, sizeof(prop_value)) > 0) {
        device_info.api_level = atoi(prop_value);
    }

    android_get_prop("ro.board.platform", device_info.board, sizeof(device_info.board));
    android_get_prop("ro.product.brand", device_info.brand, sizeof(device_info.brand));
    android_get_prop("ro.product.model", device_info.model, sizeof(device_info.model));
    android_get_prop("ro.product.manufacturer", device_info.manufacturer, sizeof(device_info.manufacturer));
    android_get_prop("ro.product.name", device_info.product, sizeof(device_info.product));

    // 為每個可用驅動創建平台條目
    for (int i = 0; i < driver_count && *platform_count < max_platforms; i++) {
        if (drivers[i].is_available) {
            retryix_platform_t* platform = &platforms[*platform_count];

            strncpy(platform->vendor, drivers[i].vendor, sizeof(platform->vendor) - 1);
            snprintf(platform->name, sizeof(platform->name), "Android %s GPU", drivers[i].vendor);
            snprintf(platform->version, sizeof(platform->version), "OpenCL on Android API %d", device_info.api_level);
            strncpy(platform->profile, "OpenCL", sizeof(platform->profile) - 1);
            strncpy(platform->extensions, device_info.extensions, sizeof(platform->extensions) - 1);
            platform->device_count = 1; // 通常一個GPU
            platform->id = (void*)(intptr_t)(*platform_count); // 簡單的ID

            (*platform_count)++;
        }
    }

    return (*platform_count > 0) ? RETRYIX_SUCCESS : RETRYIX_ERROR_NO_DEVICE;
}

// ===== Android SVM支援實現 =====

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_check_svm_support(
    bool* supports_svm,
    bool* supports_fine_grain,
    bool* supports_atomics
) {
    if (!supports_svm || !supports_fine_grain || !supports_atomics) {
        return RETRYIX_ERROR_NULL_PTR;
    }

    // 檢測Android設備GPU類型
    retryix_android_device_info_t device_info;
    retryix_android_gpu_type_t gpu_type;

    retryix_result_t result = retryix_android_detect_gpu_type(&gpu_type, &device_info);
    if (result != RETRYIX_SUCCESS) {
        *supports_svm = false;
        *supports_fine_grain = false;
        *supports_atomics = false;
        return result;
    }

    // 基於GPU類型判斷SVM支援
    switch (gpu_type) {
        case RETRYIX_ANDROID_GPU_ADRENO:
            // Adreno 6xx系列有較好的SVM支援
            *supports_svm = true;
            *supports_fine_grain = true; // 部分型號支援
            *supports_atomics = true;
            break;

        case RETRYIX_ANDROID_GPU_MALI:
            // Mali Bifrost/Valhall架構支援SVM
            *supports_svm = true;
            *supports_fine_grain = false; // 大多數只支援粗粒度
            *supports_atomics = true;
            break;

        case RETRYIX_ANDROID_GPU_POWERVR:
            // PowerVR Series8/9支援SVM
            *supports_svm = true;
            *supports_fine_grain = false;
            *supports_atomics = true;
            break;

        case RETRYIX_ANDROID_GPU_TEGRA:
            // NVIDIA Tegra有良好的SVM支援
            *supports_svm = true;
            *supports_fine_grain = true;
            *supports_atomics = true;
            break;

        default:
            *supports_svm = false;
            *supports_fine_grain = false;
            *supports_atomics = false;
            break;
    }

    return RETRYIX_SUCCESS;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_svm_alloc(
    size_t size,
    void** ptr,
    bool use_ion_allocator
) {
    if (!ptr || size == 0) return RETRYIX_ERROR_NULL_PTR;

    if (use_ion_allocator) {
        // 嘗試使用Android ION分配器（需要特殊權限）
        // 這裡使用簡化實現，實際應用中需要更複雜的ION操作
        *ptr = malloc(size);
    } else {
        // 使用標準分配
        *ptr = malloc(size);
    }

    return (*ptr != NULL) ? RETRYIX_SUCCESS : RETRYIX_ERROR_OUT_OF_MEMORY;
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_svm_free(void* ptr) {
    if (!ptr) return RETRYIX_ERROR_NULL_PTR;

    free(ptr);
    return RETRYIX_SUCCESS;
}

// ===== Android GPU性能管理 =====

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_set_gpu_performance_mode(
    int performance_level
) {
    // Android GPU性能模式設置（需要root權限）
    const char* performance_files[] = {
        "/sys/class/kgsl/kgsl-3d0/devfreq/governor",      // Adreno
        "/sys/devices/platform/mali.0/dvfs_governor",     // Mali
        "/sys/devices/platform/pvrsrvkm.0/dvfs_governor"  // PowerVR
    };

    const char* governors[] = {"powersave", "ondemand", "performance"};

    if (performance_level < 0 || performance_level > 2) {
        return RETRYIX_ERROR_INVALID_DEVICE;
    }

    for (int i = 0; i < 3; i++) {
        FILE* file = fopen(performance_files[i], "w");
        if (file) {
            fprintf(file, "%s", governors[performance_level]);
            fclose(file);
            return RETRYIX_SUCCESS; // 只要有一個成功就行
        }
    }

    return RETRYIX_ERROR_UNKNOWN; // 無法設置性能模式
}

RETRYIX_API retryix_result_t RETRYIX_CALL retryix_android_get_gpu_thermal_info(
    float* temperature_celsius,
    int* current_frequency_mhz,
    int* max_frequency_mhz
) {
    if (!temperature_celsius || !current_frequency_mhz || !max_frequency_mhz) {
        return RETRYIX_ERROR_NULL_PTR;
    }

    // 初始化默認值
    *temperature_celsius = 0.0f;
    *current_frequency_mhz = 0;
    *max_frequency_mhz = 0;

    // 嘗試讀取GPU溫度（Adreno）
    FILE* temp_file = fopen("/sys/class/thermal/thermal_zone*/temp", "r");
    if (temp_file) {
        int temp_millidegrees;
        if (fscanf(temp_file, "%d", &temp_millidegrees) == 1) {
            *temperature_celsius = temp_millidegrees / 1000.0f;
        }
        fclose(temp_file);
    }

    // 嘗試讀取GPU頻率
    FILE* freq_file = fopen("/sys/class/kgsl/kgsl-3d0/devfreq/cur_freq", "r");
    if (freq_file) {
        int freq_hz;
        if (fscanf(freq_file, "%d", &freq_hz) == 1) {
            *current_frequency_mhz = freq_hz / 1000000;
        }
        fclose(freq_file);
    }

    FILE* max_freq_file = fopen("/sys/class/kgsl/kgsl-3d0/devfreq/max_freq", "r");
    if (max_freq_file) {
        int max_freq_hz;
        if (fscanf(max_freq_file, "%d", &max_freq_hz) == 1) {
            *max_frequency_mhz = max_freq_hz / 1000000;
        }
        fclose(max_freq_file);
    }

    return RETRYIX_SUCCESS;
}