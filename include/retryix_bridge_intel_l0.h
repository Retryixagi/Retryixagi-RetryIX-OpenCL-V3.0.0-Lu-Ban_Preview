// retryix_bridge_intel_l0.h
// Intel oneAPI Level Zero 平台橋接工具頭文件
#ifndef RETRYIX_BRIDGE_INTEL_L0_H
#define RETRYIX_BRIDGE_INTEL_L0_H

#include "retryix_device.h"
#include "retryix_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 查詢所有可用 Intel Level Zero 平台（動態載入支援）
 * @param platforms 平台結構陣列（輸出）
 * @param max_platforms 陣列最大長度
 * @param platform_count 實際查得平台數
 * @return 成功 RETRYIX_SUCCESS，失敗回錯誤碼
 *
 * 支援 Xe、Arc、Data Center GPU，動態偵測驅動與型號。
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_intel_l0_platforms(
    retryix_platform_t* platforms,
    int max_platforms,
    int* platform_count
);

/**
 * @brief 分配 Level Zero USM 共享記憶體（ZeroCopy用途）
 * @param bytes 分配大小
 * @param alignment 對齊（預設4096）
 * @param out_ptr 輸出指標
 * @return 0成功，負值失敗
 */
RETRYIX_API int RETRYIX_CALL retryix_intel_l0_alloc_shared(
    size_t bytes,
    size_t alignment,
    void** out_ptr
);

#ifdef __cplusplus
}
#endif

#endif // RETRYIX_BRIDGE_INTEL_L0_H
