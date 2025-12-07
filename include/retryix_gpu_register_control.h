/*
 * retryix_gpu_register_control.h
 * Layer 0: 真正的 GPU 硬體寄存器直接控制
 * 
 * 透過 PCIe BAR (Base Address Register) Memory Mapping
 * 直接讀寫 GPU 寄存器，完全不依賴驅動 API
 * 
 * 整合 retryix_bus_scheduler.h 的 PCIe 控制能力
 */

#ifndef RETRYIX_GPU_REGISTER_CONTROL_H
#define RETRYIX_GPU_REGISTER_CONTROL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "retryix_export.h"
#include "retryix_bus_scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===================== AMD GPU 寄存器偏移定義 =====================

// === GRBM (Graphics Register Bus Manager) ===
#define AMD_GRBM_STATUS                 0x8010  // GPU 狀態寄存器
#define AMD_GRBM_STATUS2                0x8014  // GPU 擴展狀態
#define AMD_GRBM_SOFT_RESET             0x8020  // 軟重置控制

// === CP (Command Processor) ===
#define AMD_CP_RB_BASE                  0xC100  // Ring Buffer 基址
#define AMD_CP_RB_CNTL                  0xC104  // Ring Buffer 控制
#define AMD_CP_RB_RPTR                  0xC108  // Ring Buffer 讀指針
#define AMD_CP_RB_WPTR                  0xC10C  // Ring Buffer 寫指針
#define AMD_CP_RB_SIZE                  0xC110  // Ring Buffer 大小
#define AMD_CP_ME_CNTL                  0xC114  // Micro Engine 控制
#define AMD_CP_QUEUE_PRIORITY           0xC118  // Queue 優先級

// === SDMA (System DMA Engine) ===
#define AMD_SDMA0_BASE                  0xD000  // SDMA Engine 0
#define AMD_SDMA1_BASE                  0xD800  // SDMA Engine 1
#define AMD_SDMA_RB_BASE                0x0000  // 相對 SDMA_BASE
#define AMD_SDMA_RB_RPTR                0x0004
#define AMD_SDMA_RB_WPTR                0x0008
#define AMD_SDMA_RB_SIZE                0x000C
#define AMD_SDMA_CNTL                   0x0010

// === MC (Memory Controller) ===
#define AMD_MC_VM_FB_LOCATION           0x2024  // Framebuffer 位置
#define AMD_MC_VM_AGP_BASE              0x2028  // AGP 基址
#define AMD_MC_VM_AGP_TOP               0x202C  // AGP 頂部
#define AMD_MC_VM_SYSTEM_APERTURE_LOW   0x2030  // 系統 Aperture 低位
#define AMD_MC_VM_SYSTEM_APERTURE_HIGH  0x2034  // 系統 Aperture 高位

// === GFX Compute (GFX10/Navi10 正確 offset) ===
#define AMD_COMPUTE_DISPATCH_INITIATOR  0x2E00  // Compute Dispatch 控制
#define AMD_COMPUTE_PGM_LO              0x2E0C  // Shader program low address (256-byte aligned)
#define AMD_COMPUTE_PGM_HI              0x2E10  // Shader program high address
#define AMD_COMPUTE_PGM_RSRC1           0x2E12  // Resource 1 (VGPRS, SGPRS, FLOAT_MODE)
#define AMD_COMPUTE_PGM_RSRC2           0x2E13  // Resource 2 (SCRATCH, USER_SGPR, TGID)
#define AMD_COMPUTE_USER_DATA_0         0x2E40  // User data (kernel args) base
#define AMD_COMPUTE_RESOURCE_LIMITS     0x2E15  // Resource limits
#define AMD_COMPUTE_NUM_THREAD_X        0x2E1C  // Threads per group X
#define AMD_COMPUTE_NUM_THREAD_Y        0x2E1D  // Threads per group Y  (修正：原 0x2E20 → 0x2E1D)
#define AMD_COMPUTE_NUM_THREAD_Z        0x2E1E  // Threads per group Z  (修正：原 0x2E24 → 0x2E1E)
#define AMD_COMPUTE_TMPRING_SIZE        0x2E18  // Scratch ring buffer size
#define AMD_COMPUTE_SHADER_CHKSUM       0x2E24  // Shader checksum

// === Clock & Power ===
#define AMD_CG_CGTT_DRM_CLK_CTRL0       0x300   // 時鐘控制
#define AMD_CG_CGTT_DRM_CLK_CTRL1       0x304
#define AMD_PWR_CG_THERMAL_CTRL         0x320   // 熱管理控制
#define AMD_PWR_DISP_CLK_FREQ           0x324   // 顯示時鐘頻率

// ===================== PCIe BAR 配置 =====================

/**
 * AMD GPU PCIe BAR 映射
 */
typedef enum {
    RETRYIX_GPU_BAR0 = 0,  ///< MMIO 寄存器 (通常 256KB-16MB)
    RETRYIX_GPU_BAR1 = 1,  ///< 擴展寄存器 (某些 GPU)
    RETRYIX_GPU_BAR2 = 2,  ///< VRAM Aperture (可直接存取顯存)
    RETRYIX_GPU_BAR5 = 5   ///< Doorbell/Queue (用於 HSA/ROCm)
} retryix_gpu_bar_t;

/**
 * GPU 硬體控制器句柄
 */
typedef struct {
    void* bar0_mmio;              ///< BAR0 映射地址 (MMIO 寄存器)
    void* bar2_vram;              ///< BAR2 映射地址 (VRAM Aperture)
    void* bar5_doorbell;          ///< BAR5 映射地址 (Doorbell)
    
    size_t bar0_size;             ///< BAR0 大小
    size_t bar2_size;             ///< BAR2 大小 (VRAM 可見窗口)
    size_t bar5_size;             ///< BAR5 大小
    
    uint32_t pci_bus;             ///< PCI 總線號
    uint32_t pci_device;          ///< PCI 設備號
    uint32_t pci_function;        ///< PCI 功能號
    
    uint16_t vendor_id;           ///< 廠商 ID (AMD = 0x1002)
    uint16_t device_id;           ///< 設備 ID
    
    char device_name[128];        ///< GPU 名稱 (如 "AMD Radeon RX 5700 XT")
    
    // 整合匯流排調度器信息
    retryix_bus_info_t bus_info;  ///< PCIe 匯流排配置
    
    bool initialized;             ///< 是否已初始化
    void* platform_handle;        ///< Windows: HANDLE, Linux: fd
} retryix_gpu_hw_handle_t;

/**
 * GPU 命令處理器 Ring Buffer 狀態
 */
typedef struct {
    uint64_t base_address;        ///< Ring Buffer 物理地址
    uint32_t read_ptr;            ///< 讀指針 (GPU 已處理位置)
    uint32_t write_ptr;           ///< 寫指針 (CPU 寫入位置)
    uint32_t size;                ///< Ring Buffer 大小 (bytes)
    uint32_t capacity;            ///< 容量 (entries)
    bool enabled;                 ///< 是否啟用
} retryix_gpu_ring_status_t;

/**
 * GPU VRAM 記憶體區域
 */
typedef struct {
    uint64_t physical_base;       ///< VRAM 物理基址
    uint64_t aperture_base;       ///< CPU 可見窗口基址
    size_t total_size;            ///< 總 VRAM 大小
    size_t visible_size;          ///< CPU 可直接訪問大小
    size_t used_size;             ///< 已使用大小
} retryix_gpu_vram_info_t;

/**
 * GPU 計算單元狀態
 */
typedef struct {
    uint32_t compute_units_active;  ///< 活躍的計算單元數
    uint32_t wavefronts_active;     ///< 活躍的 Wavefront 數
    uint32_t simd_units_busy;       ///< 忙碌的 SIMD 單元
    float utilization_percent;      ///< GPU 利用率
    uint32_t current_clock_mhz;     ///< 當前時鐘頻率
    uint32_t current_temp_celsius;  ///< 當前溫度
} retryix_gpu_compute_status_t;

// ===================== 核心 API =====================

/**
 * @brief 初始化 GPU 硬體控制
 * @param handle 輸出 GPU 硬體句柄
 * @param pci_bus PCI 總線號 (如果為 -1 則自動偵測)
 * @param pci_device PCI 設備號 (如果為 -1 則自動偵測)
 * @return 0 成功，非 0 錯誤碼
 * 
 * 映射 PCIe BAR 到用戶空間，建立直接硬體訪問通道
 * 整合 retryix_bus_scheduler 獲取最佳 PCIe 配置
 */
RETRYIX_API int RETRYIX_CALL
retryix_gpu_hw_init(retryix_gpu_hw_handle_t* handle, int pci_bus, int pci_device);

/**
 * @brief 關閉 GPU 硬體控制
 * @param handle GPU 硬體句柄
 */
RETRYIX_API void RETRYIX_CALL
retryix_gpu_hw_cleanup(retryix_gpu_hw_handle_t* handle);

// ===================== 寄存器訪問 API =====================

/**
 * @brief 讀取 32 位 MMIO 寄存器
 * @param handle GPU 句柄
 * @param offset 寄存器偏移 (相對 BAR0)
 * @return 寄存器值
 */
RETRYIX_API uint32_t RETRYIX_CALL
retryix_gpu_read_reg32(retryix_gpu_hw_handle_t* handle, uint32_t offset);

/**
 * @brief 寫入 32 位 MMIO 寄存器
 * @param handle GPU 句柄
 * @param offset 寄存器偏移
 * @param value 寫入值
 */
RETRYIX_API void RETRYIX_CALL
retryix_gpu_write_reg32(retryix_gpu_hw_handle_t* handle, uint32_t offset, uint32_t value);

/**
 * @brief 讀取 64 位 MMIO 寄存器
 * @param handle GPU 句柄
 * @param offset 寄存器偏移
 * @return 寄存器值
 */
RETRYIX_API uint64_t RETRYIX_CALL
retryix_gpu_read_reg64(retryix_gpu_hw_handle_t* handle, uint32_t offset);

/**
 * @brief 寫入 64 位 MMIO 寄存器
 * @param handle GPU 句柄
 * @param offset 寄存器偏移
 * @param value 寫入值
 */
RETRYIX_API void RETRYIX_CALL
retryix_gpu_write_reg64(retryix_gpu_hw_handle_t* handle, uint32_t offset, uint64_t value);

/**
 * @brief 修改寄存器位域
 * @param handle GPU 句柄
 * @param offset 寄存器偏移
 * @param mask 位遮罩
 * @param value 新值
 */
RETRYIX_API void RETRYIX_CALL
retryix_gpu_modify_reg(retryix_gpu_hw_handle_t* handle, uint32_t offset, 
                       uint32_t mask, uint32_t value);

// ===================== VRAM 直接訪問 API =====================

/**
 * @brief 獲取 VRAM 信息
 * @param handle GPU 句柄
 * @param info 輸出 VRAM 信息
 * @return 0 成功
 */
RETRYIX_API int RETRYIX_CALL
retryix_gpu_get_vram_info(retryix_gpu_hw_handle_t* handle, retryix_gpu_vram_info_t* info);

/**
 * @brief 直接讀取 VRAM (透過 BAR2 Aperture)
 * @param handle GPU 句柄
 * @param vram_offset VRAM 偏移地址
 * @param buffer 輸出緩衝區
 * @param size 讀取大小
 * @return 實際讀取字節數
 */
RETRYIX_API size_t RETRYIX_CALL
retryix_gpu_vram_read(retryix_gpu_hw_handle_t* handle, uint64_t vram_offset,
                      void* buffer, size_t size);

/**
 * @brief 直接寫入 VRAM (透過 BAR2 Aperture)
 * @param handle GPU 句柄
 * @param vram_offset VRAM 偏移地址
 * @param buffer 數據緩衝區
 * @param size 寫入大小
 * @return 實際寫入字節數
 */
RETRYIX_API size_t RETRYIX_CALL
retryix_gpu_vram_write(retryix_gpu_hw_handle_t* handle, uint64_t vram_offset,
                       const void* buffer, size_t size);

/**
 * @brief 獲取 VRAM 指針 (Zero Copy 直接映射)
 * @param handle GPU 句柄
 * @param vram_offset VRAM 偏移
 * @param size 映射大小
 * @return CPU 可直接訪問的指針
 * 
 * ⚠️ 注意：返回的指針直接指向 GPU VRAM，讀寫會直接影響 GPU 內容
 */
RETRYIX_API void* RETRYIX_CALL
retryix_gpu_vram_map(retryix_gpu_hw_handle_t* handle, uint64_t vram_offset, size_t size);

// ===================== Command Processor 控制 API =====================

/**
 * @brief 獲取 Ring Buffer 狀態
 * @param handle GPU 句柄
 * @param ring_id Ring Buffer ID (通常 0 = Graphics, 1-7 = Compute)
 * @param status 輸出狀態
 * @return 0 成功
 */
RETRYIX_API int RETRYIX_CALL
retryix_gpu_get_ring_status(retryix_gpu_hw_handle_t* handle, int ring_id,
                            retryix_gpu_ring_status_t* status);

/**
 * @brief 寫入 PM4 命令包到 Ring Buffer
 * @param handle GPU 句柄
 * @param ring_id Ring Buffer ID
 * @param packet PM4 命令包數據
 * @param packet_dwords 命令包大小 (DWORDs)
 * @return 0 成功
 * 
 * PM4 (Privilege Mode 4) 是 AMD GPU 的命令語言
 * 可以直接控制 GPU 管線、記憶體、計算等
 */
RETRYIX_API int RETRYIX_CALL
retryix_gpu_submit_pm4_packet(retryix_gpu_hw_handle_t* handle, int ring_id,
                              const uint32_t* packet, size_t packet_dwords);

/**
 * @brief 更新 Ring Buffer 寫指針 (通知 GPU 有新命令)
 * @param handle GPU 句柄
 * @param ring_id Ring Buffer ID
 * @param new_wptr 新寫指針位置
 * @return 0 成功
 */
RETRYIX_API int RETRYIX_CALL
retryix_gpu_ring_doorbell(retryix_gpu_hw_handle_t* handle, int ring_id, uint32_t new_wptr);

// ===================== 計算 Dispatch API =====================

/**
 * @brief 直接 Dispatch 計算 Kernel (不透過驅動)
 * @param handle GPU 句柄
 * @param kernel_code GPU ISA 代碼地址
 * @param workgroup_x 工作組 X 維度
 * @param workgroup_y 工作組 Y 維度
 * @param workgroup_z 工作組 Z 維度
 * @param thread_x 每工作組線程數 X
 * @param thread_y 每工作組線程數 Y
 * @param thread_z 每工作組線程數 Z
 * @param kernel_args 內核參數地址
 * @return 0 成功
 * 
 * 直接寫入 COMPUTE_* 寄存器並觸發 Dispatch
 */
RETRYIX_API int RETRYIX_CALL
retryix_gpu_dispatch_compute(retryix_gpu_hw_handle_t* handle,
                             uint64_t kernel_code,
                             uint32_t workgroup_x, uint32_t workgroup_y, uint32_t workgroup_z,
                             uint32_t thread_x, uint32_t thread_y, uint32_t thread_z,
                             uint64_t kernel_args);

/**
 * @brief 等待計算完成
 * @param handle GPU 句柄
 * @param timeout_ms 超時時間 (毫秒，0 = 無限等待)
 * @return 0 完成，-1 超時
 */
RETRYIX_API int RETRYIX_CALL
retryix_gpu_wait_compute_idle(retryix_gpu_hw_handle_t* handle, uint32_t timeout_ms);

// ===================== GPU 狀態監控 API =====================

/**
 * @brief 獲取 GPU 計算狀態
 * @param handle GPU 句柄
 * @param status 輸出狀態
 * @return 0 成功
 */
RETRYIX_API int RETRYIX_CALL
retryix_gpu_get_compute_status(retryix_gpu_hw_handle_t* handle, 
                               retryix_gpu_compute_status_t* status);

/**
 * @brief 檢查 GPU 是否空閒
 * @param handle GPU 句柄
 * @return true 空閒，false 忙碌
 */
RETRYIX_API bool RETRYIX_CALL
retryix_gpu_is_idle(retryix_gpu_hw_handle_t* handle);

/**
 * @brief 重置 GPU (軟重置)
 * @param handle GPU 句柄
 * @return 0 成功
 * 
 * ⚠️ 危險操作！會終止所有 GPU 活動
 */
RETRYIX_API int RETRYIX_CALL
retryix_gpu_soft_reset(retryix_gpu_hw_handle_t* handle);

// ===================== 時鐘與電源控制 API =====================

/**
 * @brief 設置 GPU 時鐘頻率
 * @param handle GPU 句柄
 * @param clock_mhz 目標頻率 (MHz)
 * @return 0 成功
 */
RETRYIX_API int RETRYIX_CALL
retryix_gpu_set_clock(retryix_gpu_hw_handle_t* handle, uint32_t clock_mhz);

/**
 * @brief 獲取當前 GPU 時鐘頻率
 * @param handle GPU 句柄
 * @return 頻率 (MHz)
 */
RETRYIX_API uint32_t RETRYIX_CALL
retryix_gpu_get_clock(retryix_gpu_hw_handle_t* handle);

/**
 * @brief 設置電源管理模式
 * @param handle GPU 句柄
 * @param high_performance true = 高性能，false = 節能
 * @return 0 成功
 */
RETRYIX_API int RETRYIX_CALL
retryix_gpu_set_power_mode(retryix_gpu_hw_handle_t* handle, bool high_performance);

// ===================== DMA 引擎控制 API =====================

/**
 * @brief 使用 SDMA 引擎執行記憶體拷貝
 * @param handle GPU 句柄
 * @param dst_addr 目標地址 (VRAM 或系統記憶體)
 * @param src_addr 源地址
 * @param size 拷貝大小
 * @return 0 成功
 * 
 * SDMA (System DMA) 可以異步拷貝數據，不占用 CPU
 */
RETRYIX_API int RETRYIX_CALL
retryix_gpu_sdma_copy(retryix_gpu_hw_handle_t* handle,
                     uint64_t dst_addr, uint64_t src_addr, size_t size);

/**
 * @brief 等待 SDMA 完成
 * @param handle GPU 句柄
 * @param engine_id SDMA 引擎 ID (0 或 1)
 * @return 0 成功
 */
RETRYIX_API int RETRYIX_CALL
retryix_gpu_sdma_wait(retryix_gpu_hw_handle_t* handle, int engine_id);

// ===================== 工具函數 =====================

/**
 * @brief 枚舉系統中所有 AMD GPU
 * @param handles 輸出句柄數組
 * @param max_count 最大數量
 * @return 實際找到的 GPU 數量
 */
RETRYIX_API int RETRYIX_CALL
retryix_gpu_enumerate_devices(retryix_gpu_hw_handle_t* handles, int max_count);

/**
 * @brief 驗證寄存器訪問權限
 * @param handle GPU 句柄
 * @return true 可訪問，false 權限不足
 */
RETRYIX_API bool RETRYIX_CALL
retryix_gpu_verify_access(retryix_gpu_hw_handle_t* handle);

/**
 * @brief 格式化 GPU 信息為字符串
 * @param handle GPU 句柄
 * @param buffer 輸出緩衝區
 * @param buffer_size 緩衝區大小
 * @return 格式化長度
 */
RETRYIX_API int RETRYIX_CALL
retryix_gpu_format_info(const retryix_gpu_hw_handle_t* handle, char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* RETRYIX_GPU_REGISTER_CONTROL_H */
