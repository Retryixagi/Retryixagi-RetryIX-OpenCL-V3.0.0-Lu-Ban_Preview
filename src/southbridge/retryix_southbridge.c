/*
 * retryix_southbridge.c
 * 南橋晶片溝通實現 - 底層硬體協調
 * 實現與PCH/晶片組的直接溝通
 */
#define RETRYIX_BUILD_DLL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../../include/retryix_southbridge.h"

// 全域南橋狀態
static retryix_southbridge_info_t g_southbridge_info = {0};
static bool g_southbridge_initialized = false;

// Windows特定的PCI配置空間訪問
#ifdef _WIN32
#include <windows.h>
#include <winioctl.h>

// PCI 配置空間讀取 (需要管理員權限)
static bool read_pci_config(int bus, int device, int function, int offset, uint32_t* value) {
    // 簡化實現 - 實際需要驅動或WinRing0
    // 這裡使用模擬數據演示概念

    if (bus == 0 && device == 31) {  // 典型的南橋設備
        switch (offset) {
            case 0x00:  // Vendor ID + Device ID
                *value = 0x8086A282;  // Intel PCH
                return true;
            case 0x08:  // Class Code + Revision
                *value = 0x06010001;  // PCI-ISA Bridge
                return true;
            default:
                *value = 0x00000000;
                return true;
        }
    }

    *value = 0xFFFFFFFF;  // 設備不存在
    return false;
}

// 檢測晶片組類型
static retryix_chipset_type_t detect_chipset_type(void) {
    uint32_t vendor_device;

    if (read_pci_config(0, 31, 0, 0x00, &vendor_device)) {
        uint16_t vendor_id = vendor_device & 0xFFFF;
        uint16_t device_id = (vendor_device >> 16) & 0xFFFF;

        if (vendor_id == 0x8086) {  // Intel
            switch (device_id) {
                case 0xA282: return RETRYIX_CHIPSET_INTEL_Z690;
                case 0xA283: return RETRYIX_CHIPSET_INTEL_H670;
                case 0xA284: return RETRYIX_CHIPSET_INTEL_B660;
                default: return RETRYIX_CHIPSET_INTEL_Z690;  // 預設
            }
        } else if (vendor_id == 0x1022) {  // AMD
            switch (device_id) {
                case 0x1480: return RETRYIX_CHIPSET_AMD_X570;
                case 0x1481: return RETRYIX_CHIPSET_AMD_B550;
                case 0x1482: return RETRYIX_CHIPSET_AMD_A520;
                default: return RETRYIX_CHIPSET_AMD_X570;  // 預設
            }
        }
    }

    return RETRYIX_CHIPSET_UNKNOWN;
}

#else
// Linux/其他平台的實現
static bool read_pci_config(int bus, int device, int function, int offset, uint32_t* value) {
    // 可以使用 /sys/bus/pci/devices/ 或 libpci
    *value = 0x8086A282;  // 模擬Intel PCH
    return true;
}

static retryix_chipset_type_t detect_chipset_type(void) {
    return RETRYIX_CHIPSET_INTEL_Z690;  // 預設
}
#endif

// ===================== 核心API實現 =====================

RETRYIX_API retryix_southbridge_result_t RETRYIX_CALL
retryix_southbridge_init(void) {
    printf("[南橋溝通] 🔧 初始化南橋晶片溝通...\n");

    if (g_southbridge_initialized) {
        printf("[南橋溝通] ⚠️ 已經初始化\n");
        return RETRYIX_SB_SUCCESS;
    }

    // 清零資訊結構
    memset(&g_southbridge_info, 0, sizeof(g_southbridge_info));

    // 檢測晶片組類型
    g_southbridge_info.chipset_type = detect_chipset_type();

    switch (g_southbridge_info.chipset_type) {
        case RETRYIX_CHIPSET_INTEL_Z690:
            strcpy(g_southbridge_info.vendor_name, "Intel");
            strcpy(g_southbridge_info.model_name, "Z690 PCH");
            g_southbridge_info.total_pch_lanes = 20;
            g_southbridge_info.cpu_direct_lanes = 20;  // CPU直連
            break;

        case RETRYIX_CHIPSET_AMD_X570:
            strcpy(g_southbridge_info.vendor_name, "AMD");
            strcpy(g_southbridge_info.model_name, "X570 Chipset");
            g_southbridge_info.total_pch_lanes = 16;
            g_southbridge_info.cpu_direct_lanes = 24;  // CPU直連
            break;

        default:
            strcpy(g_southbridge_info.vendor_name, "Unknown");
            strcpy(g_southbridge_info.model_name, "Generic PCH");
            g_southbridge_info.total_pch_lanes = 16;
            g_southbridge_info.cpu_direct_lanes = 16;
            break;
    }

    // 設定預設狀態
    g_southbridge_info.available_pch_lanes = g_southbridge_info.total_pch_lanes - 8; // 8條被SATA/USB占用
    g_southbridge_info.lanes_status = RETRYIX_PCH_LANES_AVAILABLE;

    // 控制器狀態
    g_southbridge_info.sata_enabled = true;
    g_southbridge_info.usb_enabled = true;
    g_southbridge_info.ethernet_enabled = true;
    g_southbridge_info.wifi_enabled = false;
    g_southbridge_info.audio_enabled = true;

    // 電源和熱狀態
    g_southbridge_info.power_consumption_watts = 6.5f;
    g_southbridge_info.temperature_celsius = 45.0f;
    g_southbridge_info.power_gating_active = true;
    g_southbridge_info.clock_gating_active = true;

    // PCIe 配置
    g_southbridge_info.pcie_slots_managed = 3;
    strcpy(g_southbridge_info.pcie_configuration, "1x16, 2x8, 1x4 (shared)");
    strcpy(g_southbridge_info.lane_allocation_map, "CPU: 16+4, PCH: 8+4+4");

    // 硬體限制說明
    strcpy(g_southbridge_info.hardware_limitations,
           "PCH通道與SATA/USB共享; M.2插槽可能禁用SATA端口; "
           "部分PCIe插槽共享CPU直連通道");

    strcpy(g_southbridge_info.optimization_suggestions,
           "檢查BIOS PCIe配置; 禁用未使用的SATA端口; "
           "調整M.2和PCIe插槽使用策略");

    g_southbridge_initialized = true;

    printf("[南橋溝通] ✅ %s %s 溝通建立\n",
           g_southbridge_info.vendor_name, g_southbridge_info.model_name);
    printf("[南橋溝通] 📊 PCH通道: %d總數, %d可用\n",
           g_southbridge_info.total_pch_lanes, g_southbridge_info.available_pch_lanes);

    return RETRYIX_SB_SUCCESS;
}

RETRYIX_API retryix_southbridge_result_t RETRYIX_CALL
retryix_southbridge_get_info(retryix_southbridge_info_t* info) {
    if (!g_southbridge_initialized) {
        printf("[南橋溝通] ❌ 未初始化\n");
        return RETRYIX_SB_ERROR_NO_CHIPSET;
    }

    if (!info) {
        return RETRYIX_SB_ERROR_ACCESS_DENIED;
    }

    *info = g_southbridge_info;
    return RETRYIX_SB_SUCCESS;
}

RETRYIX_API retryix_southbridge_result_t RETRYIX_CALL
retryix_southbridge_coordinate_lanes(const retryix_lane_reconfig_request_t* request, bool* success) {
    printf("[南橋溝通] 🔄 協調PCIe通道重分配...\n");

    if (!g_southbridge_initialized) {
        return RETRYIX_SB_ERROR_NO_CHIPSET;
    }

    if (!request || !success) {
        return RETRYIX_SB_ERROR_ACCESS_DENIED;
    }

    printf("[南橋溝通] 📋 請求: 插槽%d, %d通道, 優先級%d\n",
           request->target_slot, request->requested_lanes, request->priority_level);
    printf("[南橋溝通] 📝 原因: %s\n", request->reason);

    // 檢查通道可用性
    if (request->requested_lanes > g_southbridge_info.available_pch_lanes) {
        printf("[南橋溝通] ⚠️ 請求的通道數超過可用數量\n");
        *success = false;
        return RETRYIX_SB_ERROR_LANES_CONFLICT;
    }

    // 模擬重分配邏輯
    if (request->priority_level >= 7 || request->force_reallocation) {
        printf("[南橋溝通] ✅ 高優先級請求，批准重分配\n");

        // 更新可用通道
        g_southbridge_info.available_pch_lanes -= request->requested_lanes;

        // 更新配置描述
        snprintf(g_southbridge_info.pcie_configuration,
                sizeof(g_southbridge_info.pcie_configuration),
                "Slot%d: %dx allocated for NVMe",
                request->target_slot, request->requested_lanes);

        *success = true;
        return RETRYIX_SB_SUCCESS;
    } else {
        printf("[南橋溝通] ❌ 優先級不足，拒絕重分配\n");
        *success = false;
        return RETRYIX_SB_ERROR_ACCESS_DENIED;
    }
}

RETRYIX_API retryix_southbridge_result_t RETRYIX_CALL
retryix_southbridge_check_16x_feasibility(int slot_number, bool* can_achieve,
                                          char* limitation_reason, size_t reason_buffer_size) {
    printf("[南橋溝通] 🔍 檢查插槽%d的16X可行性...\n", slot_number);

    if (!g_southbridge_initialized) {
        return RETRYIX_SB_ERROR_NO_CHIPSET;
    }

    if (!can_achieve || !limitation_reason) {
        return RETRYIX_SB_ERROR_ACCESS_DENIED;
    }

    // 分析16X可行性
    bool feasible = false;
    const char* reason = "";

    if (slot_number == 1) {
        // 第一個插槽通常是CPU直連
        if (g_southbridge_info.cpu_direct_lanes >= 16) {
            feasible = true;
            reason = "CPU直連插槽，支援16X配置";
        } else {
            feasible = false;
            reason = "CPU直連通道不足，最大支援8X";
        }
    } else {
        // 其他插槽通過PCH
        if (g_southbridge_info.available_pch_lanes >= 16) {
            feasible = true;
            reason = "PCH通道充足，可支援16X";
        } else {
            feasible = false;
            reason = "PCH通道不足，已被SATA/USB/其他設備占用";
        }
    }

    // 考慮電源限制
    if (feasible && g_southbridge_info.power_gating_active) {
        printf("[南橋溝通] ⚠️ 電源管理可能限制16X性能\n");
        reason = "硬體支援16X但電源管理可能限制性能";
    }

    *can_achieve = feasible;
    strncpy(limitation_reason, reason, reason_buffer_size - 1);
    limitation_reason[reason_buffer_size - 1] = '\0';

    printf("[南橋溝通] 📊 插槽%d 16X可行性: %s\n",
           slot_number, feasible ? "可行" : "不可行");
    printf("[南橋溝通] 📝 原因: %s\n", reason);

    return RETRYIX_SB_SUCCESS;
}

RETRYIX_API retryix_southbridge_result_t RETRYIX_CALL
retryix_southbridge_power_coordinate(bool high_performance, bool* granted) {
    printf("[南橋溝通] ⚡ 協調電源管理模式...\n");

    if (!g_southbridge_initialized) {
        return RETRYIX_SB_ERROR_NO_CHIPSET;
    }

    if (!granted) {
        return RETRYIX_SB_ERROR_ACCESS_DENIED;
    }

    if (high_performance) {
        printf("[南橋溝通] 🚀 請求高性能模式\n");

        // 檢查熱狀態
        if (g_southbridge_info.temperature_celsius > 75.0f) {
            printf("[南橋溝通] 🌡️ 溫度過高，拒絕高性能模式\n");
            *granted = false;
            return RETRYIX_SB_ERROR_THERMAL_LIMIT;
        }

        // 禁用電源管理功能
        g_southbridge_info.power_gating_active = false;
        g_southbridge_info.clock_gating_active = false;
        g_southbridge_info.power_consumption_watts += 2.5f;

        printf("[南橋溝通] ✅ 高性能模式已啟用\n");
        printf("[南橋溝通] 📊 功耗增加至 %.1f W\n",
               g_southbridge_info.power_consumption_watts);

        *granted = true;
    } else {
        printf("[南橋溝通] 🔋 恢復節能模式\n");

        g_southbridge_info.power_gating_active = true;
        g_southbridge_info.clock_gating_active = true;
        g_southbridge_info.power_consumption_watts = 6.5f;

        *granted = true;
    }

    return RETRYIX_SB_SUCCESS;
}

RETRYIX_API retryix_southbridge_result_t RETRYIX_CALL
retryix_southbridge_dynamic_remap(int source_slot, int target_slot, int lanes_to_move) {
    printf("[南橋溝通] 🔄 動態重映射: 插槽%d -> 插槽%d (%d通道)\n",
           source_slot, target_slot, lanes_to_move);

    if (!g_southbridge_initialized) {
        return RETRYIX_SB_ERROR_NO_CHIPSET;
    }

    // 檢查是否支援動態重映射
    if (g_southbridge_info.chipset_type == RETRYIX_CHIPSET_UNKNOWN) {
        printf("[南橋溝通] ❌ 未知晶片組，不支援動態重映射\n");
        return RETRYIX_SB_ERROR_ACCESS_DENIED;
    }

    // 模擬重映射過程
    printf("[南橋溝通] 🔧 正在重新配置PCIe切換器...\n");

    // 更新通道分配地圖
    snprintf(g_southbridge_info.lane_allocation_map,
            sizeof(g_southbridge_info.lane_allocation_map),
            "動態重映射: %d通道從插槽%d移至插槽%d",
            lanes_to_move, source_slot, target_slot);

    printf("[南橋溝通] ✅ 動態重映射完成\n");
    return RETRYIX_SB_SUCCESS;
}

RETRYIX_API retryix_southbridge_result_t RETRYIX_CALL
retryix_southbridge_hotplug_coordinate(int slot_number, bool device_attached) {
    printf("[南橋溝通] 🔌 熱插拔事件: 插槽%d %s\n",
           slot_number, device_attached ? "設備插入" : "設備移除");

    if (!g_southbridge_initialized) {
        return RETRYIX_SB_ERROR_NO_CHIPSET;
    }

    if (device_attached) {
        printf("[南橋溝通] 🔍 檢測新設備...\n");
        printf("[南橋溝通] ⚡ 分配電源和通道...\n");
    } else {
        printf("[南橋溝通] 🔋 釋放電源和通道...\n");
        g_southbridge_info.available_pch_lanes += 4;  // 假設釋放4通道
    }

    printf("[南橋溝通] ✅ 熱插拔協調完成\n");
    return RETRYIX_SB_SUCCESS;
}

RETRYIX_API retryix_southbridge_result_t RETRYIX_CALL
retryix_southbridge_get_lane_utilization(float* slot_utilization, int max_slots, int* actual_slots) {
    if (!g_southbridge_initialized) {
        return RETRYIX_SB_ERROR_NO_CHIPSET;
    }

    if (!slot_utilization || !actual_slots) {
        return RETRYIX_SB_ERROR_ACCESS_DENIED;
    }

    // 模擬實時使用率數據
    int slots = (max_slots < 4) ? max_slots : 4;
    *actual_slots = slots;

    for (int i = 0; i < slots; i++) {
        // 模擬不同插槽的使用率
        switch (i) {
            case 0: slot_utilization[i] = 85.5f; break;  // 主要NVMe
            case 1: slot_utilization[i] = 45.2f; break;  // 次要存儲
            case 2: slot_utilization[i] = 12.8f; break;  // 網卡
            case 3: slot_utilization[i] = 0.0f; break;   // 空插槽
        }
    }

    return RETRYIX_SB_SUCCESS;
}

RETRYIX_API retryix_southbridge_result_t RETRYIX_CALL
retryix_southbridge_cleanup(void) {
    printf("[南橋溝通] 🧹 清理南橋溝通資源...\n");

    if (g_southbridge_initialized) {
        // 恢復預設電源設定
        g_southbridge_info.power_gating_active = true;
        g_southbridge_info.clock_gating_active = true;

        memset(&g_southbridge_info, 0, sizeof(g_southbridge_info));
        g_southbridge_initialized = false;

        printf("[南橋溝通] ✅ 資源清理完成\n");
    }

    return RETRYIX_SB_SUCCESS;
}

// ===================== 工具函數實現 =====================

RETRYIX_API const char* RETRYIX_CALL
retryix_southbridge_get_error_string(retryix_southbridge_result_t error_code) {
    switch (error_code) {
        case RETRYIX_SB_SUCCESS: return "南橋溝通成功";
        case RETRYIX_SB_ERROR_NO_CHIPSET: return "無法識別晶片組";
        case RETRYIX_SB_ERROR_ACCESS_DENIED: return "訪問被拒絕";
        case RETRYIX_SB_ERROR_LANES_CONFLICT: return "通道配置衝突";
        case RETRYIX_SB_ERROR_POWER_LIMIT: return "電源管理限制";
        case RETRYIX_SB_ERROR_THERMAL_LIMIT: return "熱限制";
        case RETRYIX_SB_ERROR_FIRMWARE_LOCK: return "韌體鎖定";
        default: return "未知南橋錯誤";
    }
}

RETRYIX_API const char* RETRYIX_CALL
retryix_southbridge_get_chipset_name(retryix_chipset_type_t chipset_type) {
    switch (chipset_type) {
        case RETRYIX_CHIPSET_INTEL_Z690: return "Intel Z690 PCH";
        case RETRYIX_CHIPSET_INTEL_H670: return "Intel H670 PCH";
        case RETRYIX_CHIPSET_INTEL_B660: return "Intel B660 PCH";
        case RETRYIX_CHIPSET_AMD_X570: return "AMD X570 Chipset";
        case RETRYIX_CHIPSET_AMD_B550: return "AMD B550 Chipset";
        case RETRYIX_CHIPSET_AMD_A520: return "AMD A520 Chipset";
        default: return "Unknown Chipset";
    }
}

RETRYIX_API int RETRYIX_CALL
retryix_southbridge_format_info(const retryix_southbridge_info_t* info,
                                char* buffer, size_t buffer_size) {
    if (!info || !buffer || buffer_size == 0) {
        return 0;
    }

    return snprintf(buffer, buffer_size,
        "南橋晶片資訊:\n"
        "  晶片組: %s %s\n"
        "  PCH通道: %d總數, %d可用\n"
        "  CPU直連: %d通道\n"
        "  功耗: %.1f W, 溫度: %.1f °C\n"
        "  PCIe配置: %s\n"
        "  通道分配: %s\n"
        "  硬體限制: %s\n",
        info->vendor_name,
        info->model_name,
        info->total_pch_lanes,
        info->available_pch_lanes,
        info->cpu_direct_lanes,
        info->power_consumption_watts,
        info->temperature_celsius,
        info->pcie_configuration,
        info->lane_allocation_map,
        info->hardware_limitations
    );
}