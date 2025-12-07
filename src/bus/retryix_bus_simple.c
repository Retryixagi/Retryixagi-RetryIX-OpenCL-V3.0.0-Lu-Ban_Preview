
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef RETRYIX_BUILD_DLL
#define RETRYIX_API __declspec(dllexport)
#else
#define RETRYIX_API
#endif
#define RETRYIX_CALL __cdecl

// 基本類型定義
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
    RETRYIX_BUS_SUCCESS = 0,
    RETRYIX_BUS_ERROR_NO_CONTROLLERS = -1,
    RETRYIX_BUS_ERROR_INSUFFICIENT_BUFFER = -2
} retryix_bus_result_t;

typedef struct {
    int controller_id;
    char device_name[128];
    retryix_pcie_lanes_t configured_lanes;
    retryix_pcie_lanes_t max_lanes;
    retryix_nvme_generation_t generation;
    float theoretical_bandwidth_gbps;
    float actual_bandwidth_gbps;
    int power_limit_active;
    int thermal_throttling;
    char limitation_reason[512];
} retryix_bus_info_t;

// 匯流排調度API實現
RETRYIX_API retryix_bus_result_t RETRYIX_CALL retryix_bus_scheduler_init(void) {
    printf("[BUS-SCHEDULER] 匯流排調度器初始化\n");
    return RETRYIX_BUS_SUCCESS;
}

RETRYIX_API retryix_bus_result_t RETRYIX_CALL retryix_bus_enumerate_controllers(retryix_bus_info_t* controllers, int* count) {
    printf("[BUS-SCHEDULER] 枚舉匯流排控制器\n");
    if (controllers && count && *count > 0) {
        // 模擬一個Gen4 8X NVMe控制器
        controllers[0].controller_id = 0;
        strcpy(controllers[0].device_name, "Samsung NVMe SSD 980 PRO");
        controllers[0].configured_lanes = RETRYIX_BUS_PCIE_8X;
        controllers[0].max_lanes = RETRYIX_BUS_PCIE_16X;
        controllers[0].generation = RETRYIX_NVME_GEN4;
        controllers[0].theoretical_bandwidth_gbps = 15.75f;  // 8X Gen4
        controllers[0].actual_bandwidth_gbps = 12.0f;        // 實際測量值
        controllers[0].power_limit_active = 1;
        strcpy(controllers[0].limitation_reason, "電源管理限制，主機板插槽限制16X");
        *count = 1;
        return RETRYIX_BUS_SUCCESS;
    }
    return RETRYIX_BUS_ERROR_INSUFFICIENT_BUFFER;
}

RETRYIX_API retryix_bus_result_t RETRYIX_CALL retryix_bus_get_optimal_config(retryix_bus_info_t* config) {
    printf("[BUS-SCHEDULER] 獲取最佳匯流排配置\n");
    if (config) {
        config->controller_id = 0;
        strcpy(config->device_name, "最佳可用配置");
        config->configured_lanes = RETRYIX_BUS_PCIE_8X;
        config->generation = RETRYIX_NVME_GEN4;
        config->actual_bandwidth_gbps = 12.0f;
        strcpy(config->limitation_reason, "16X不可用：電源管理和主機板限制");
        return RETRYIX_BUS_SUCCESS;
    }
    return RETRYIX_BUS_ERROR_INSUFFICIENT_BUFFER;
}

RETRYIX_API retryix_bus_result_t RETRYIX_CALL retryix_bus_fallback_reason(int controller_id, char* reason_buffer, size_t buffer_size) {
    printf("[BUS-SCHEDULER] 獲取16X不可用原因\n");
    if (reason_buffer && buffer_size > 0) {
        strncpy(reason_buffer, "系統電源管理限制PCIe功耗; 主機板PCIe插槽不支援16X全速; CPU PCIe lanes與其他設備共享", buffer_size - 1);
        reason_buffer[buffer_size - 1] = '\0';
        return RETRYIX_BUS_SUCCESS;
    }
    return RETRYIX_BUS_ERROR_INSUFFICIENT_BUFFER;
}

RETRYIX_API float RETRYIX_CALL retryix_bus_calculate_theoretical_bandwidth(retryix_pcie_lanes_t lanes, retryix_nvme_generation_t generation) {
    float lane_bandwidth = 0.0f;
    switch (generation) {
        case RETRYIX_NVME_GEN3: lane_bandwidth = 0.985f; break;
        case RETRYIX_NVME_GEN4: lane_bandwidth = 1.969f; break;
        case RETRYIX_NVME_GEN5: lane_bandwidth = 3.938f; break;
        case RETRYIX_NVME_GEN6: lane_bandwidth = 7.875f; break;
        default: lane_bandwidth = 1.969f; break;
    }
    return lane_bandwidth * lanes;
}

RETRYIX_API const char* RETRYIX_CALL retryix_bus_get_error_string(retryix_bus_result_t error_code) {
    switch (error_code) {
        case RETRYIX_BUS_SUCCESS: return "Success";
        case RETRYIX_BUS_ERROR_NO_CONTROLLERS: return "No controllers found";
        case RETRYIX_BUS_ERROR_INSUFFICIENT_BUFFER: return "Insufficient buffer";
        default: return "Unknown error";
    }
}

RETRYIX_API retryix_bus_result_t RETRYIX_CALL retryix_bus_scheduler_cleanup(void) {
    printf("[BUS-SCHEDULER] 清理匯流排調度器\n");
    return RETRYIX_BUS_SUCCESS;
}
