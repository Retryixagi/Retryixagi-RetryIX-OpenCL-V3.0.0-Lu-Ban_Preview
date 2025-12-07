/*
 * retryix_bus_scheduler.h
 * 通用匯流排調度與NVMe世代支援API
 * 自動偵測16X優先，階梯退回機制
 */

#ifndef RETRYIX_BUS_SCHEDULER_H
#define RETRYIX_BUS_SCHEDULER_H

#include <stddef.h>
#include <stdbool.h>
// === RetryIX Bus Scheduler API Exports ===
#include "retryix_export.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===================== 類型定義 =====================

/**
 * PCIe通道數量枚舉
 */
typedef enum {
    RETRYIX_BUS_PCIE_1X = 1,     ///< PCIe x1 配置
    RETRYIX_BUS_PCIE_4X = 4,     ///< PCIe x4 配置
    RETRYIX_BUS_PCIE_8X = 8,     ///< PCIe x8 配置
    RETRYIX_BUS_PCIE_16X = 16    ///< PCIe x16 配置（最佳）
} retryix_pcie_lanes_t;

/**
 * NVMe/PCIe世代支援
 */
typedef enum {
    RETRYIX_NVME_GEN3 = 3,       ///< PCIe Gen3 - 8 GT/s 每通道
    RETRYIX_NVME_GEN4 = 4,       ///< PCIe Gen4 - 16 GT/s 每通道
    RETRYIX_NVME_GEN5 = 5,       ///< PCIe Gen5 - 32 GT/s 每通道
    RETRYIX_NVME_GEN6 = 6        ///< PCIe Gen6 - 64 GT/s 每通道（未來）
} retryix_nvme_generation_t;

/**
 * 匯流排調度錯誤碼
 */
typedef enum {
    RETRYIX_BUS_SUCCESS = 0,                    ///< 操作成功
    RETRYIX_BUS_ERROR_NO_CONTROLLERS = -1,     ///< 未發現控制器
    RETRYIX_BUS_ERROR_INSUFFICIENT_BUFFER = -2, ///< 緩衝區不足
    RETRYIX_BUS_ERROR_HARDWARE_ACCESS = -3,    ///< 硬體訪問失敗
    RETRYIX_BUS_ERROR_POWER_LIMITATION = -4,   ///< 電源限制
    RETRYIX_BUS_ERROR_THERMAL_THROTTLING = -5, ///< 熱節流
    RETRYIX_BUS_ERROR_INVALID_PARAMETER = -6   ///< 無效參數
} retryix_bus_result_t;

/**
 * 匯流排控制器信息結構體
 * 包含完整的硬體配置、限制狀態和性能指標
 */
typedef struct {
    // === 基本標識信息 ===
    int controller_id;                    ///< 控制器唯一標識
    char device_name[128];                ///< 設備友好名稱
    char model_name[64];                  ///< 控制器型號
    char vendor_id[16];                   ///< 廠商ID (如 "144d" for Samsung)
    char device_id[16];                   ///< 設備ID

    // === PCIe配置信息 ===
    retryix_pcie_lanes_t configured_lanes; ///< 當前配置的PCIe通道數
    retryix_pcie_lanes_t max_lanes;        ///< 硬體支援的最大通道數
    retryix_nvme_generation_t generation;  ///< PCIe世代
    int pcie_slot_number;                  ///< PCIe插槽編號

    // === 帶寬性能指標 ===
    float theoretical_bandwidth_gbps;      ///< 理論最大帶寬 (GB/s)
    float actual_bandwidth_gbps;           ///< 實際可用帶寬 (GB/s)
    float peak_measured_bandwidth_gbps;    ///< 實測峰值帶寬 (GB/s)
    int queue_depth;                       ///< NVMe隊列深度

    // === 限制狀態標誌 ===
    bool power_limit_active;               ///< 電源管理限制啟用
    bool thermal_throttling;               ///< 熱節流保護啟用
    bool motherboard_limitation;           ///< 主機板硬體限制
    bool cpu_lanes_exhausted;              ///< CPU PCIe lanes耗盡
    bool nvme_queue_depth_limited;          ///< NVMe隊列深度受限
    bool aspm_enabled;                     ///< 鏈路電源管理啟用

    // === 詳細狀態信息 ===
    char limitation_reason[512];           ///< 16X不可用的詳細原因
    char power_management_info[256];       ///< 電源管理詳細信息
    char thermal_info[128];                ///< 熱狀態信息
    char performance_notes[256];           ///< 性能調優建議

    // === 運行時統計 ===
    unsigned long long total_bytes_transferred; ///< 累計傳輸字節數
    unsigned int error_count;              ///< 錯誤計數
    float average_latency_us;              ///< 平均延遲 (微秒)
    float utilization_percentage;          ///< 帶寬利用率百分比

} retryix_bus_info_t;

/**
 * 優化配置建議結構體
 */
typedef struct {
    retryix_bus_info_t best_controller;    ///< 推薦的最佳控制器
    bool can_achieve_16x;                  ///< 是否可以達到16X
    char optimization_suggestions[512];    ///< 性能優化建議
    char bios_settings_recommendations[256]; ///< BIOS設置建議
} retryix_bus_optimization_t;

// ===================== 核心API函數 =====================

/**
 * @brief 初始化匯流排調度器
 * @return 操作結果碼
 *
 * 初始化匯流排調度系統，枚舉所有可用的NVMe控制器，
 * 分析硬體配置和限制狀態。
 */
RETRYIX_API retryix_bus_result_t RETRYIX_CALL
retryix_bus_scheduler_init(void);

/**
 * @brief 枚舉所有匯流排控制器
 * @param controllers 輸出控制器信息數組
 * @param count 輸入：數組大小；輸出：實際控制器數量
 * @return 操作結果碼
 *
 * 獲取系統中所有NVMe控制器的詳細信息，包括PCIe配置、
 * 性能指標和限制狀態。
 */
RETRYIX_API retryix_bus_result_t RETRYIX_CALL
retryix_bus_enumerate_controllers(retryix_bus_info_t* controllers, int* count);

/**
 * @brief 獲取最佳控制器配置
 * @param config 輸出最佳配置信息
 * @return 操作結果碼
 *
 * 自動選擇最佳的NVMe控制器配置，優先選擇16X Gen5，
 * 如不可用則按階梯順序退回到次優配置。
 */
RETRYIX_API retryix_bus_result_t RETRYIX_CALL
retryix_bus_get_optimal_config(retryix_bus_info_t* config);

/**
 * @brief 獲取16X不可用的詳細原因
 * @param controller_id 控制器ID
 * @param reason_buffer 輸出原因字符串緩衝區
 * @param buffer_size 緩衝區大小
 * @return 操作結果碼
 *
 * 當系統無法達到16X配置時，提供詳細的原因說明，
 * 幫助用戶理解硬體限制和可能的解決方案。
 */
RETRYIX_API retryix_bus_result_t RETRYIX_CALL
retryix_bus_fallback_reason(int controller_id, char* reason_buffer, size_t buffer_size);

/**
 * @brief 獲取優化建議
 * @param controller_id 控制器ID
 * @param optimization 輸出優化建議
 * @return 操作結果碼
 *
 * 分析當前硬體配置，提供具體的性能優化建議，
 * 包括BIOS設置、驅動更新等。
 */
RETRYIX_API retryix_bus_result_t RETRYIX_CALL
retryix_bus_get_optimization_suggestions(int controller_id, retryix_bus_optimization_t* optimization);

// ===================== 高級功能API =====================

/**
 * @brief 執行帶寬基準測試
 * @param controller_id 控制器ID
 * @param test_size_mb 測試數據大小 (MB)
 * @param measured_bandwidth 輸出實測帶寬 (GB/s)
 * @return 操作結果碼
 *
 * 對指定控制器執行實際帶寬測試，驗證理論值與實際性能的差異。
 */
RETRYIX_API retryix_bus_result_t RETRYIX_CALL
retryix_bus_benchmark_bandwidth(int controller_id, int test_size_mb, float* measured_bandwidth);

/**
 * @brief 設置性能模式
 * @param controller_id 控制器ID
 * @param high_performance 是否啟用高性能模式
 * @return 操作結果碼
 *
 * 調整控制器的性能設置，在高性能和節能之間切換。
 * 注意：高性能模式可能增加功耗和發熱。
 */
RETRYIX_API retryix_bus_result_t RETRYIX_CALL
retryix_bus_set_performance_mode(int controller_id, bool high_performance);

/**
 * @brief 監控控制器狀態
 * @param controller_id 控制器ID
 * @param status 輸出當前狀態
 * @return 操作結果碼
 *
 * 實時監控控制器的運行狀態，包括溫度、帶寬利用率、
 * 錯誤統計等信息。
 */
RETRYIX_API retryix_bus_result_t RETRYIX_CALL
retryix_bus_monitor_status(int controller_id, retryix_bus_info_t* status);

/**
 * @brief 清理匯流排調度器資源
 * @return 操作結果碼
 *
 * 清理匯流排調度器使用的所有資源，釋放記憶體。
 */
RETRYIX_API retryix_bus_result_t RETRYIX_CALL
retryix_bus_scheduler_cleanup(void);

// ===================== 工具函數 =====================

/**
 * @brief 獲取錯誤信息字符串
 * @param error_code 錯誤碼
 * @return 錯誤描述字符串
 */
RETRYIX_API const char* RETRYIX_CALL
retryix_bus_get_error_string(retryix_bus_result_t error_code);

/**
 * @brief 計算理論帶寬
 * @param lanes PCIe通道數
 * @param generation PCIe世代
 * @return 理論帶寬 (GB/s)
 */
RETRYIX_API float RETRYIX_CALL
retryix_bus_calculate_theoretical_bandwidth(retryix_pcie_lanes_t lanes,
                                           retryix_nvme_generation_t generation);

/**
 * @brief 格式化控制器信息為字符串
 * @param info 控制器信息
 * @param buffer 輸出緩衝區
 * @param buffer_size 緩衝區大小
 * @return 格式化的字符串長度
 */
RETRYIX_API int RETRYIX_CALL
retryix_bus_format_controller_info(const retryix_bus_info_t* info,
                                   char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* RETRYIX_BUS_SCHEDULER_H */