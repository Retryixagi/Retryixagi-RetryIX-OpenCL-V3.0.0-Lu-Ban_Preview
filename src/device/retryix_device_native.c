// retryix_device_native.c
// 原生硬體偵測 - 分層回退策略: Vulkan → DXGI → Registry
// RetryIX 3.0.0 魯班

#include "retryix_core.h"
#include "retryix_device.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Vulkan 支持 (動態加載,可選)
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

// Vulkan 函數指針
typedef VkResult (VKAPI_PTR *PFN_vkCreateInstance_)(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*);
typedef void (VKAPI_PTR *PFN_vkDestroyInstance_)(VkInstance, const VkAllocationCallbacks*);
typedef VkResult (VKAPI_PTR *PFN_vkEnumeratePhysicalDevices_)(VkInstance, uint32_t*, VkPhysicalDevice*);
typedef void (VKAPI_PTR *PFN_vkGetPhysicalDeviceProperties_)(VkPhysicalDevice, VkPhysicalDeviceProperties*);
typedef void (VKAPI_PTR *PFN_vkGetPhysicalDeviceMemoryProperties_)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties*);

static PFN_vkCreateInstance_ vkCreateInstance_dyn = NULL;
static PFN_vkDestroyInstance_ vkDestroyInstance_dyn = NULL;
static PFN_vkEnumeratePhysicalDevices_ vkEnumeratePhysicalDevices_dyn = NULL;
static PFN_vkGetPhysicalDeviceProperties_ vkGetPhysicalDeviceProperties_dyn = NULL;
static PFN_vkGetPhysicalDeviceMemoryProperties_ vkGetPhysicalDeviceMemoryProperties_dyn = NULL;

#ifdef _WIN32
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

// Windows 註冊表硬體查詢
static int query_windows_gpu_registry(retryix_device_t* devices, int max_devices) {
    int count = 0;
    HKEY hKey;
    
    // 查詢 Display 設備
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
        "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        char subkey_name[256];
        DWORD index = 0;
        
        while (RegEnumKeyA(hKey, index++, subkey_name, sizeof(subkey_name)) == ERROR_SUCCESS) {
            if (strcmp(subkey_name, "Properties") == 0) continue;
            
            HKEY hSubKey;
            if (RegOpenKeyExA(hKey, subkey_name, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
                char driver_desc[256] = {0};
                char hardware_id[256] = {0};
                DWORD type, size;
                
                // 讀取設備描述
                size = sizeof(driver_desc);
                RegQueryValueExA(hSubKey, "DriverDesc", NULL, &type, (LPBYTE)driver_desc, &size);
                
                // 讀取硬體 ID
                size = sizeof(hardware_id);
                RegQueryValueExA(hSubKey, "MatchingDeviceId", NULL, &type, (LPBYTE)hardware_id, &size);
                
                if (strlen(driver_desc) > 0 && count < max_devices) {
                    retryix_device_t* d = &devices[count];
                    memset(d, 0, sizeof(*d));
                    d->struct_size = sizeof(retryix_device_t);
                    d->struct_version = 0x00010000;
                    
                    strncpy(d->name, driver_desc, RETRYIX_MAX_NAME_LEN - 1);
                    
                    // 判斷供應商
                    if (strstr(hardware_id, "VEN_1002") || strstr(driver_desc, "AMD") || strstr(driver_desc, "Radeon")) {
                        strncpy(d->vendor, "Advanced Micro Devices, Inc.", RETRYIX_MAX_NAME_LEN - 1);
                        d->custom_flags |= 0x01; // AMD flag
                        
                        // 檢測 RX 5000 系列
                        if (strstr(driver_desc, "RX 5") || strstr(driver_desc, "5700") || strstr(driver_desc, "5600")) {
                            d->is_amd_rx5000 = 1;
                        }
                    } else if (strstr(hardware_id, "VEN_10DE") || strstr(driver_desc, "NVIDIA") || strstr(driver_desc, "GeForce")) {
                        strncpy(d->vendor, "NVIDIA Corporation", RETRYIX_MAX_NAME_LEN - 1);
                        d->custom_flags |= 0x02; // NVIDIA flag
                    } else if (strstr(hardware_id, "VEN_8086") || strstr(driver_desc, "Intel")) {
                        strncpy(d->vendor, "Intel Corporation", RETRYIX_MAX_NAME_LEN - 1);
                    }
                    
                    strncpy(d->version, "Native Detection 1.0", RETRYIX_MAX_VERSION_LEN - 1);
                    d->is_available = 1;
                    d->performance_score = 50.0f;
                    
                    count++;
                }
                
                RegCloseKey(hSubKey);
            }
        }
        
        RegCloseKey(hKey);
    }
    
    return count;
}

// Vulkan 硬體查詢 (跨平台,優先使用)
static int query_vulkan_gpu(retryix_device_t* devices, int max_devices) {
    int count = 0;
    
#ifdef _WIN32
    HMODULE vulkan_lib = LoadLibraryA("vulkan-1.dll");
    if (!vulkan_lib) return 0;
    
    vkCreateInstance_dyn = (PFN_vkCreateInstance_)GetProcAddress(vulkan_lib, "vkCreateInstance");
    vkDestroyInstance_dyn = (PFN_vkDestroyInstance_)GetProcAddress(vulkan_lib, "vkDestroyInstance");
    vkEnumeratePhysicalDevices_dyn = (PFN_vkEnumeratePhysicalDevices_)GetProcAddress(vulkan_lib, "vkEnumeratePhysicalDevices");
    vkGetPhysicalDeviceProperties_dyn = (PFN_vkGetPhysicalDeviceProperties_)GetProcAddress(vulkan_lib, "vkGetPhysicalDeviceProperties");
    vkGetPhysicalDeviceMemoryProperties_dyn = (PFN_vkGetPhysicalDeviceMemoryProperties_)GetProcAddress(vulkan_lib, "vkGetPhysicalDeviceMemoryProperties");
    
    if (!vkCreateInstance_dyn || !vkEnumeratePhysicalDevices_dyn || !vkGetPhysicalDeviceProperties_dyn) {
        FreeLibrary(vulkan_lib);
        return 0;
    }
#endif
    
    // 創建 Vulkan 實例
    VkApplicationInfo app_info;
    memset(&app_info, 0, sizeof(app_info));
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = "RetryIX";
    app_info.applicationVersion = VK_MAKE_VERSION(3, 0, 0);
    app_info.pEngineName = "LuBan";
    app_info.engineVersion = VK_MAKE_VERSION(3, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;
    
    VkInstanceCreateInfo create_info;
    memset(&create_info, 0, sizeof(create_info));
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = NULL;
    create_info.enabledExtensionCount = 0;
    create_info.ppEnabledExtensionNames = NULL;
    
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance_dyn(&create_info, NULL, &instance);
    if (result != VK_SUCCESS) {
#ifdef _WIN32
        FreeLibrary(vulkan_lib);
#endif
        return 0;
    }
    
    // 枚舉物理設備
    uint32_t device_count = 0;
    result = vkEnumeratePhysicalDevices_dyn(instance, &device_count, NULL);
    
    if (result != VK_SUCCESS || device_count == 0) {
        vkDestroyInstance_dyn(instance, NULL);
#ifdef _WIN32
        FreeLibrary(vulkan_lib);
#endif
        return 0;
    }
    
    if (device_count > (uint32_t)max_devices) {
        device_count = (uint32_t)max_devices;
    }
    
    VkPhysicalDevice* physical_devices = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * device_count);
    if (!physical_devices) {
        vkDestroyInstance_dyn(instance, NULL);
#ifdef _WIN32
        FreeLibrary(vulkan_lib);
#endif
        return 0;
    }
    
    result = vkEnumeratePhysicalDevices_dyn(instance, &device_count, physical_devices);
    if (result != VK_SUCCESS) {
        free(physical_devices);
        vkDestroyInstance_dyn(instance, NULL);
#ifdef _WIN32
        FreeLibrary(vulkan_lib);
#endif
        return 0;
    }
    
    for (uint32_t i = 0; i < device_count; i++) {
        VkPhysicalDeviceProperties props;
        VkPhysicalDeviceMemoryProperties mem_props;
        
        vkGetPhysicalDeviceProperties_dyn(physical_devices[i], &props);
        vkGetPhysicalDeviceMemoryProperties_dyn(physical_devices[i], &mem_props);
        
        retryix_device_t* d = &devices[count];
        memset(d, 0, sizeof(*d));
        d->struct_size = sizeof(retryix_device_t);
        d->struct_version = 0x00010000;
        
        // 設備名稱
        strncpy(d->name, props.deviceName, RETRYIX_MAX_NAME_LEN - 1);
        
        // 判斷供應商
        if (props.vendorID == 0x1002) { // AMD
            strncpy(d->vendor, "Advanced Micro Devices, Inc.", RETRYIX_MAX_NAME_LEN - 1);
            d->custom_flags |= 0x01;
            if (strstr(props.deviceName, "RX 5") || strstr(props.deviceName, "5700")) {
                d->is_amd_rx5000 = 1;
            }
        } else if (props.vendorID == 0x10DE) { // NVIDIA
            strncpy(d->vendor, "NVIDIA Corporation", RETRYIX_MAX_NAME_LEN - 1);
            d->custom_flags |= 0x02;
        } else if (props.vendorID == 0x8086) { // Intel
            strncpy(d->vendor, "Intel Corporation", RETRYIX_MAX_NAME_LEN - 1);
        }
        
        // 計算總 VRAM (所有 DEVICE_LOCAL 內存堆)
        cl_ulong total_vram = 0;
        for (uint32_t j = 0; j < mem_props.memoryHeapCount; j++) {
            if (mem_props.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                total_vram += mem_props.memoryHeaps[j].size;
            }
        }
        d->global_memory = total_vram;
        
        // Vulkan API 版本
        snprintf(d->version, RETRYIX_MAX_VERSION_LEN, "Vulkan %d.%d.%d", 
            VK_VERSION_MAJOR(props.apiVersion),
            VK_VERSION_MINOR(props.apiVersion),
            VK_VERSION_PATCH(props.apiVersion));
        
        d->is_available = 1;
        d->performance_score = 80.0f; // Vulkan 獲取的信息最完整
        
        count++;
    }
    
    free(physical_devices);
    vkDestroyInstance_dyn(instance, NULL);
    
#ifdef _WIN32
    FreeLibrary(vulkan_lib);
#endif
    
    return count;
}

// DXGI 硬體查詢 (Windows 10+)
#ifdef _WIN32
#include <dxgi.h>
#include <initguid.h>
DEFINE_GUID(IID_IDXGIFactory_Local, 0x7b7166ec, 0x21c7, 0x44ae, 0xb2, 0x1a, 0xc9, 0xae, 0x32, 0x1a, 0xe3, 0x69);

static int query_windows_gpu_dxgi(retryix_device_t* devices, int max_devices) {
    int count = 0;
    IDXGIFactory* pFactory = NULL;
    
    if (CreateDXGIFactory(&IID_IDXGIFactory_Local, (void**)&pFactory) != S_OK) {
        return 0;
    }
    
    IDXGIAdapter* pAdapter = NULL;
    UINT i = 0;
    
    while (pFactory->lpVtbl->EnumAdapters(pFactory, i, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
        if (count >= max_devices) break;
        
        DXGI_ADAPTER_DESC desc;
        if (pAdapter->lpVtbl->GetDesc(pAdapter, &desc) == S_OK) {
            retryix_device_t* d = &devices[count];
            memset(d, 0, sizeof(*d));
            d->struct_size = sizeof(retryix_device_t);
            d->struct_version = 0x00010000;
            
            // 轉換寬字符到多字節
            WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, 
                d->name, RETRYIX_MAX_NAME_LEN, NULL, NULL);
            
            // 判斷供應商 (通過 VendorId)
            if (desc.VendorId == 0x1002) { // AMD
                strncpy(d->vendor, "Advanced Micro Devices, Inc.", RETRYIX_MAX_NAME_LEN - 1);
                d->custom_flags |= 0x01;
                if (strstr(d->name, "RX 5") || strstr(d->name, "5700") || strstr(d->name, "5600")) {
                    d->is_amd_rx5000 = 1;
                }
            } else if (desc.VendorId == 0x10DE) { // NVIDIA
                strncpy(d->vendor, "NVIDIA Corporation", RETRYIX_MAX_NAME_LEN - 1);
                d->custom_flags |= 0x02;
            } else if (desc.VendorId == 0x8086) { // Intel
                strncpy(d->vendor, "Intel Corporation", RETRYIX_MAX_NAME_LEN - 1);
            }
            
            // 顯存大小 - 從註冊表讀取真實硬體容量
            d->global_memory = desc.DedicatedVideoMemory;
            
            // 嘗試從註冊表讀取準確的 VRAM 容量
            HKEY hKey;
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}",
                0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                
                char subkey_name[256];
                DWORD index = 0;
                
                while (RegEnumKeyA(hKey, index++, subkey_name, sizeof(subkey_name)) == ERROR_SUCCESS) {
                    HKEY hSubKey;
                    if (RegOpenKeyExA(hKey, subkey_name, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
                        char check_desc[256] = {0};
                        DWORD type, size = sizeof(check_desc);
                        RegQueryValueExA(hSubKey, "DriverDesc", NULL, &type, (LPBYTE)check_desc, &size);
                        
                        // 匹配當前設備
                        if (strlen(check_desc) > 0 && strstr(d->name, check_desc)) {
                            // 讀取 HardwareInformation.qwMemorySize (真實 VRAM 大小)
                            DWORD64 vram_size = 0;
                            size = sizeof(vram_size);
                            if (RegQueryValueExA(hSubKey, "HardwareInformation.qwMemorySize", NULL, &type, 
                                (LPBYTE)&vram_size, &size) == ERROR_SUCCESS) {
                                if (vram_size > 0) {
                                    d->global_memory = vram_size;
                                }
                            }
                            // 也可以嘗試 HardwareInformation.MemorySize (DWORD)
                            if (d->global_memory == desc.DedicatedVideoMemory) {
                                DWORD vram_size_dw = 0;
                                size = sizeof(vram_size_dw);
                                if (RegQueryValueExA(hSubKey, "HardwareInformation.MemorySize", NULL, &type,
                                    (LPBYTE)&vram_size_dw, &size) == ERROR_SUCCESS) {
                                    if (vram_size_dw > 0) {
                                        d->global_memory = (cl_ulong)vram_size_dw;
                                    }
                                }
                            }
                        }
                        
                        RegCloseKey(hSubKey);
                    }
                }
                RegCloseKey(hKey);
            }
            
            strncpy(d->version, "DXGI Native Detection", RETRYIX_MAX_VERSION_LEN - 1);
            d->is_available = 1;
            d->performance_score = 50.0f;
            
            count++;
        }
        
        pAdapter->lpVtbl->Release(pAdapter);
        i++;
    }
    
    pFactory->lpVtbl->Release(pFactory);
    return count;
}
#endif

#elif defined(__linux__)

#include <dirent.h>
#include <unistd.h>

// Linux sysfs 硬體查詢
static int query_linux_gpu_sysfs(retryix_device_t* devices, int max_devices) {
    int count = 0;
    DIR* dir = opendir("/sys/class/drm");
    if (!dir) return 0;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && count < max_devices) {
        if (strncmp(entry->d_name, "card", 4) == 0) {
            char path[512];
            snprintf(path, sizeof(path), "/sys/class/drm/%s/device/uevent", entry->d_name);
            
            FILE* f = fopen(path, "r");
            if (f) {
                retryix_device_t* d = &devices[count];
                memset(d, 0, sizeof(*d));
                d->struct_size = sizeof(retryix_device_t);
                d->struct_version = 0x00010000;
                
                char line[256];
                while (fgets(line, sizeof(line), f)) {
                    if (strncmp(line, "PCI_ID=", 7) == 0) {
                        unsigned int vendor, device;
                        sscanf(line + 7, "%X:%X", &vendor, &device);
                        
                        if (vendor == 0x1002) {
                            strncpy(d->vendor, "Advanced Micro Devices, Inc.", RETRYIX_MAX_NAME_LEN - 1);
                            d->custom_flags |= 0x01;
                        } else if (vendor == 0x10DE) {
                            strncpy(d->vendor, "NVIDIA Corporation", RETRYIX_MAX_NAME_LEN - 1);
                            d->custom_flags |= 0x02;
                        } else if (vendor == 0x8086) {
                            strncpy(d->vendor, "Intel Corporation", RETRYIX_MAX_NAME_LEN - 1);
                        }
                        
                        snprintf(d->name, RETRYIX_MAX_NAME_LEN, "GPU Device %04X:%04X", vendor, device);
                    }
                }
                
                strncpy(d->version, "Linux Native Detection", RETRYIX_MAX_VERSION_LEN - 1);
                d->is_available = 1;
                d->performance_score = 50.0f;
                
                fclose(f);
                count++;
            }
        }
    }
    
    closedir(dir);
    return count;
}

#endif

// 統一的原生硬體偵測接口 - 分層回退策略
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_devices_native(
    retryix_device_t* devices,
    int max_devices,
    int* device_count
) {
    if (!devices || !device_count || max_devices <= 0) {
        return RETRYIX_ERROR_INVALID_PARAMETER;
    }
    
    *device_count = 0;
    int count = 0;
    
    // 第 1 層: 嘗試 Vulkan (最詳細,跨平台)
    count = query_vulkan_gpu(devices, max_devices);
    if (count > 0) {
        *device_count = count;
        return RETRYIX_SUCCESS;
    }
    
#ifdef _WIN32
    // 第 2 層: 嘗試 DXGI (Windows 原生,較詳細)
    count = query_windows_gpu_dxgi(devices, max_devices);
    if (count > 0) {
        *device_count = count;
        return RETRYIX_SUCCESS;
    }
    
    // 第 3 層: 降級到註冊表 (最基本)
    count = query_windows_gpu_registry(devices, max_devices);
    *device_count = count;
    
#elif defined(__linux__)
    // Linux: 嘗試 sysfs
    count = query_linux_gpu_sysfs(devices, max_devices);
    *device_count = count;
    
#else
    return RETRYIX_ERROR_NOT_SUPPORTED;
#endif
    
    return (*device_count > 0) ? RETRYIX_SUCCESS : RETRYIX_ERROR_NO_DEVICE;
}
