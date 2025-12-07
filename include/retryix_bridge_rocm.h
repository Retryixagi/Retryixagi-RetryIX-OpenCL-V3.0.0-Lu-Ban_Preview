// retryix_bridge_rocm.h
// ROCm/AMD GPU 平台橋接工具頭文件
#ifndef RETRYIX_BRIDGE_ROCM_H
#define RETRYIX_BRIDGE_ROCM_H

#include "retryix_device.h"
#include "retryix_core.h"

// 支援動態載入多版本 libhip_hcc.so、librocblas.so、Windows/AMD 驅動
// 平台查詢自動辨識 GCN/Polaris/Vega/Navi/RDNA/RDNA2/RDNA3/9000系列等
// 錯誤碼：RETRYIX_ERROR_NO_PLATFORM, RETRYIX_ERROR_NULL_PTR, RETRYIX_ERROR_BACKEND_FAILURE

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 查詢所有可用 ROCm/AMD GPU 平台（動態載入支援）
 * @param platforms 平台結構陣列（輸出）
 * @param max_platforms 陣列最大長度
 * @param platform_count 實際查得平台數
 * @return 成功 RETRYIX_SUCCESS，失敗回錯誤碼
 *
 * 支援無 ROCm 開發環境、僅驅動或多版本情境。
 * platform name 會顯示型號、GCN/RDNA世代、UI描述。
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_rocm_platforms(
    retryix_platform_t* platforms,
    int max_platforms,
    int* platform_count
);

#ifdef __cplusplus
}
#endif

#endif // RETRYIX_BRIDGE_ROCM_H
