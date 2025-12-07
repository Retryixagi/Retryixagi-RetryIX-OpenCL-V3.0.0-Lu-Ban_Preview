// retryix_bridge_opencl_intel.h
// Intel OpenCL 平台橋接工具頭文件
#ifndef RETRYIX_BRIDGE_OPENCL_INTEL_H
#define RETRYIX_BRIDGE_OPENCL_INTEL_H

#include "retryix_device.h"
#include "retryix_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 查詢所有可用 Intel OpenCL 平台（動態載入支援）
 * @param platforms 平台結構陣列（輸出）
 * @param max_platforms 陣列最大長度
 * @param platform_count 實際查得平台數
 * @return 成功 RETRYIX_SUCCESS，失敗回錯誤碼
 *
 * 支援 USM/SVM 能力偵測，Xe/Arc/核顯/FPGA 皆可。
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_opencl_intel_platforms(
    retryix_platform_t* platforms,
    int max_platforms,
    int* platform_count
);

#ifdef __cplusplus
}
#endif

#endif // RETRYIX_BRIDGE_OPENCL_INTEL_H
