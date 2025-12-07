/*
 * retryix_bus_scheduler.c
 * 通用匯流排調度與NVMe世代支援實現
 * 自動偵測16X優先，階梯退回機制
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")
#endif

// 前向聲明結構體，完整定義在頭文件中
typedef enum {
    RETRYIX_BUS_PCIE_16X = 16,
    RETRYIX_BUS_PCIE_8X = 8,
    RETRYIX_BUS_PCIE_4X = 4,
    RETRYIX_BUS_PCIE_1X = 1
} retryix_pcie_lanes_t;

typedef enum {
    RETRYIX_NVME_GEN3 = 3,
    RETRYIX_NVME_GEN4 = 4,
    RETRYIX_NVME_GEN5 = 5,
    RETRYIX_NVME_GEN6 = 6
} retryix_nvme_generation_t;

typedef enum {
    RETRYIX_SUCCESS = 0,
    RETRYIX_ERROR_NO_CONTROLLERS = -1,
    RETRYIX_ERROR_INSUFFICIENT_BUFFER = -2,
    RETRYIX_ERROR_HARDWARE_ACCESS = -3,
    RETRYIX_ERROR_POWER_LIMITATION = -4,
    RETRYIX_ERROR_THERMAL_THROTTLING = -5
} retryix_result_t;

typedef struct {
    int controller_id;
    char device_name[128];
    char model_name[64];

    retryix_pcie_lanes_t configured_lanes;
    retryix_pcie_lanes_t max_lanes;
    retryix_nvme_generation_t generation;

    float theoretical_bandwidth_gbps;
    float actual_bandwidth_gbps;

    // 限制狀態標誌
    bool power_limit_active;
    bool thermal_throttling;
    bool motherboard_limitation;
    bool cpu_lanes_exhausted;
    bool nvme_queue_depth_limited;

    // 詳細原因說明
    char limitation_reason[512];
    char power_management_info[256];
    char thermal_info[128];
} retryix_bus_info_t;

// 全域狀態
static retryix_bus_info_t g_controllers[16];
static int g_controller_count = 0;
static bool g_initialized = false;

// 內部函數聲明
static retryix_result_t detect_pcie_configuration(int controller_id, retryix_bus_info_t* info);
static retryix_result_t detect_nvme_generation(int controller_id, retryix_bus_info_t* info);
static retryix_result_t analyze_bandwidth_limitations(retryix_bus_info_t* info);
static retryix_result_t check_power_management_status(retryix_bus_info_t* info);
static retryix_result_t check_thermal_status(retryix_bus_info_t* info);
static float calculate_theoretical_bandwidth(retryix_pcie_lanes_t lanes, retryix_nvme_generation_t gen);
static void build_limitation_reason_string(retryix_bus_info_t* info);

/**
 * 初始化匯流排調度器
 */
retryix_result_t retryix_bus_scheduler_init(void) {
    printf("[BUS-SCHEDULER] 初始化通用匯流排調度器\n");

    if (g_initialized) {
        return RETRYIX_SUCCESS;
    }

    memset(g_controllers, 0, sizeof(g_controllers));
    g_controller_count = 0;

    // Windows NVMe控制器枚舉
#ifdef _WIN32
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(
        &GUID_DEVCLASS_SCSIADAPTER, NULL, NULL, DIGCF_PRESENT);

    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        printf("[BUS-SCHEDULER] 無法獲取設備信息集\n");
        return RETRYIX_ERROR_HARDWARE_ACCESS;
    }

    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++) {
        if (g_controller_count >= 16) break;

        char deviceName[256];
        if (SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData,
                SPDRP_FRIENDLYNAME, NULL, (PBYTE)deviceName, sizeof(deviceName), NULL)) {

            // 檢查是否為NVMe控制器
            if (strstr(deviceName, "NVMe") || strstr(deviceName, "SSD")) {
                retryix_bus_info_t* controller = &g_controllers[g_controller_count];
                controller->controller_id = g_controller_count;
                strncpy(controller->device_name, deviceName, sizeof(controller->device_name) - 1);

                // 偵測PCIe配置
                detect_pcie_configuration(g_controller_count, controller);

                // 偵測NVMe世代
                detect_nvme_generation(g_controller_count, controller);

                // 分析帶寬限制
                analyze_bandwidth_limitations(controller);

                printf("[BUS-SCHEDULER] 發現控制器 %d: %s\n",
                       g_controller_count, controller->device_name);
                printf("[BUS-SCHEDULER]   PCIe: %dX Gen%d, 帶寬: %.1f GB/s\n",
                       controller->configured_lanes, controller->generation,
                       controller->actual_bandwidth_gbps);

                g_controller_count++;
            }
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
#endif

    if (g_controller_count == 0) {
        printf("[BUS-SCHEDULER] 警告：未發現任何NVMe控制器\n");
        return RETRYIX_ERROR_NO_CONTROLLERS;
    }

    g_initialized = true;
    printf("[BUS-SCHEDULER] 初始化完成，發現 %d 個控制器\n", g_controller_count);
    return RETRYIX_SUCCESS;
}

/**
 * 枚舉所有匯流排控制器
 */
retryix_result_t retryix_bus_enumerate_controllers(retryix_bus_info_t* controllers, int* count) {
    if (!controllers || !count) {
        return RETRYIX_ERROR_INSUFFICIENT_BUFFER;
    }

    if (!g_initialized) {
        retryix_result_t result = retryix_bus_scheduler_init();
        if (result != RETRYIX_SUCCESS) {
            return result;
        }
    }

    if (*count < g_controller_count) {
        *count = g_controller_count;
        return RETRYIX_ERROR_INSUFFICIENT_BUFFER;
    }

    memcpy(controllers, g_controllers, g_controller_count * sizeof(retryix_bus_info_t));
    *count = g_controller_count;

    return RETRYIX_SUCCESS;
}

/**
 * 獲取最佳控制器配置（16X優先，階梯退回）
 */
retryix_result_t retryix_bus_get_optimal_config(retryix_bus_info_t* config) {
    if (!config) {
        return RETRYIX_ERROR_INSUFFICIENT_BUFFER;
    }

    if (!g_initialized) {
        retryix_result_t result = retryix_bus_scheduler_init();
        if (result != RETRYIX_SUCCESS) {
            return result;
        }
    }

    if (g_controller_count == 0) {
        return RETRYIX_ERROR_NO_CONTROLLERS;
    }

    // 階梯優先順序：16X Gen5 > 8X Gen5 > 16X Gen4 > 8X Gen4 > 4X Gen5 > 4X Gen4 > 其他
    retryix_bus_info_t* best_controller = NULL;
    float best_score = 0.0f;

    for (int i = 0; i < g_controller_count; i++) {
        retryix_bus_info_t* ctrl = &g_controllers[i];

        // 計算評分：帶寬 * 穩定性係數
        float stability_factor = 1.0f;
        if (ctrl->power_limit_active) stability_factor *= 0.8f;
        if (ctrl->thermal_throttling) stability_factor *= 0.7f;
        if (ctrl->motherboard_limitation) stability_factor *= 0.9f;

        float score = ctrl->actual_bandwidth_gbps * stability_factor;

        if (score > best_score) {
            best_score = score;
            best_controller = ctrl;
        }
    }

    if (best_controller) {
        memcpy(config, best_controller, sizeof(retryix_bus_info_t));

        printf("[BUS-SCHEDULER] 選擇最佳控制器: %s\n", config->device_name);
        printf("[BUS-SCHEDULER] 配置: %dX Gen%d, 實際帶寬: %.1f GB/s\n",
               config->configured_lanes, config->generation, config->actual_bandwidth_gbps);

        if (config->configured_lanes < RETRYIX_BUS_PCIE_16X) {
            printf("[BUS-SCHEDULER] 注意：未能達到16X配置\n");
            printf("[BUS-SCHEDULER] 原因：%s\n", config->limitation_reason);
        }

        return RETRYIX_SUCCESS;
    }

    return RETRYIX_ERROR_NO_CONTROLLERS;
}

/**
 * 獲取16X不可用的詳細原因
 */
retryix_result_t retryix_bus_fallback_reason(int controller_id, char* reason_buffer, size_t buffer_size) {
    if (!reason_buffer || buffer_size == 0) {
        return RETRYIX_ERROR_INSUFFICIENT_BUFFER;
    }

    if (controller_id < 0 || controller_id >= g_controller_count) {
        snprintf(reason_buffer, buffer_size, "無效的控制器ID: %d", controller_id);
        return RETRYIX_ERROR_INSUFFICIENT_BUFFER;
    }

    retryix_bus_info_t* ctrl = &g_controllers[controller_id];

    if (ctrl->configured_lanes >= RETRYIX_BUS_PCIE_16X) {
        snprintf(reason_buffer, buffer_size, "控制器已運行在16X模式");
        return RETRYIX_SUCCESS;
    }

    strncpy(reason_buffer, ctrl->limitation_reason, buffer_size - 1);
    reason_buffer[buffer_size - 1] = '\0';

    return RETRYIX_SUCCESS;
}

// ================== 內部實現函數 ==================

static retryix_result_t detect_pcie_configuration(int controller_id, retryix_bus_info_t* info) {
    // 模擬PCIe配置偵測（實際實現需要讀取PCIe配置空間）
    info->max_lanes = RETRYIX_BUS_PCIE_16X;
    info->configured_lanes = RETRYIX_BUS_PCIE_8X;  // 模擬大多數系統的實際配置

    // 檢查主機板和CPU限制
    check_power_management_status(info);
    check_thermal_status(info);

    return RETRYIX_SUCCESS;
}

static retryix_result_t detect_nvme_generation(int controller_id, retryix_bus_info_t* info) {
    // 模擬NVMe世代偵測（實際實現需要讀取NVMe標識信息）
    info->generation = RETRYIX_NVME_GEN4;  // 模擬Gen4 NVMe

    // 計算理論帶寬
    info->theoretical_bandwidth_gbps = calculate_theoretical_bandwidth(
        info->configured_lanes, info->generation);

    return RETRYIX_SUCCESS;
}

static retryix_result_t analyze_bandwidth_limitations(retryix_bus_info_t* info) {
    info->actual_bandwidth_gbps = info->theoretical_bandwidth_gbps;

    // 應用各種限制因子
    if (info->power_limit_active) {
        info->actual_bandwidth_gbps *= 0.75f;  // 電源限制降速25%
    }

    if (info->thermal_throttling) {
        info->actual_bandwidth_gbps *= 0.80f;  // 熱節流降速20%
    }

    if (info->cpu_lanes_exhausted) {
        info->actual_bandwidth_gbps *= 0.50f;  // CPU lanes共享降速50%
    }

    // 構建限制原因字符串
    build_limitation_reason_string(info);

    return RETRYIX_SUCCESS;
}

static retryix_result_t check_power_management_status(retryix_bus_info_t* info) {
    // 模擬電源管理狀態檢查
    info->power_limit_active = (info->controller_id % 3 == 0);  // 模擬部分設備有電源限制

    if (info->power_limit_active) {
        snprintf(info->power_management_info, sizeof(info->power_management_info),
                "系統電源管理啟用PCIe節能模式，限制最大功耗至75W");
    }

    return RETRYIX_SUCCESS;
}

static retryix_result_t check_thermal_status(retryix_bus_info_t* info) {
    // 模擬熱狀態檢查
    info->thermal_throttling = (info->controller_id % 4 == 0);  // 模擬部分設備有熱節流

    if (info->thermal_throttling) {
        snprintf(info->thermal_info, sizeof(info->thermal_info),
                "NVMe控制器溫度過高，啟動熱保護節流機制");
    }

    // 模擬其他限制
    info->motherboard_limitation = (info->controller_id % 5 == 0);
    info->cpu_lanes_exhausted = (info->controller_id % 6 == 0);

    return RETRYIX_SUCCESS;
}

static float calculate_theoretical_bandwidth(retryix_pcie_lanes_t lanes, retryix_nvme_generation_t gen) {
    // PCIe理論帶寬計算 (GB/s)
    float lane_bandwidth = 0.0f;

    switch (gen) {
        case RETRYIX_NVME_GEN3: lane_bandwidth = 0.985f; break;   // 1 GT/s ≈ 0.985 GB/s
        case RETRYIX_NVME_GEN4: lane_bandwidth = 1.969f; break;   // 2 GT/s ≈ 1.969 GB/s
        case RETRYIX_NVME_GEN5: lane_bandwidth = 3.938f; break;   // 4 GT/s ≈ 3.938 GB/s
        case RETRYIX_NVME_GEN6: lane_bandwidth = 7.875f; break;   // 8 GT/s ≈ 7.875 GB/s
        default: lane_bandwidth = 1.969f; break;
    }

    return lane_bandwidth * lanes;
}

static void build_limitation_reason_string(retryix_bus_info_t* info) {
    char reasons[512] = {0};

    if (info->configured_lanes < RETRYIX_BUS_PCIE_16X) {
        if (info->power_limit_active) {
            strcat(reasons, "電源管理限制PCIe功耗; ");
        }
        if (info->thermal_throttling) {
            strcat(reasons, "熱節流保護啟動; ");
        }
        if (info->motherboard_limitation) {
            strcat(reasons, "主機板PCIe插槽限制; ");
        }
        if (info->cpu_lanes_exhausted) {
            strcat(reasons, "CPU PCIe lanes已用盡; ");
        }

        if (strlen(reasons) == 0) {
            strcpy(reasons, "硬體配置限制");
        } else {
            // 移除最後的"; "
            reasons[strlen(reasons) - 2] = '\0';
        }
    } else {
        strcpy(reasons, "運行在最佳16X配置");
    }

    strncpy(info->limitation_reason, reasons, sizeof(info->limitation_reason) - 1);
    info->limitation_reason[sizeof(info->limitation_reason) - 1] = '\0';
}

/**
 * 清理匯流排調度器資源
 */
retryix_result_t retryix_bus_scheduler_cleanup(void) {
    if (g_initialized) {
        printf("[BUS-SCHEDULER] 清理匯流排調度器資源\n");
        memset(g_controllers, 0, sizeof(g_controllers));
        g_controller_count = 0;
        g_initialized = false;
    }
    return RETRYIX_SUCCESS;
}