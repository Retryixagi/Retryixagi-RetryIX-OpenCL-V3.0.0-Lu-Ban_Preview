// retryix_bridge_cpu_intel.h
// Intel CPU 平台橋接工具頭文件
#ifndef RETRYIX_BRIDGE_CPU_INTEL_H
#define RETRYIX_BRIDGE_CPU_INTEL_H

#include "retryix_device.h"
#include "retryix_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 查詢本機 Intel CPU 平台（偵測指令集能力）
 * @param platforms 平台結構陣列（輸出）
 * @param max_platforms 陣列最大長度
 * @param platform_count 實際查得平台數
 * @return 成功 RETRYIX_SUCCESS，失敗回錯誤碼
 *
 * 支援 AVX2/AVX512F/AMX 指令集偵測，Xeon/桌機/筆電皆可。
 */
RETRYIX_API retryix_result_t RETRYIX_CALL retryix_discover_cpu_intel(
    retryix_platform_t* platforms,
    int max_platforms,
    int* platform_count
);

#ifdef __cplusplus
}
#endif

#endif // RETRYIX_BRIDGE_CPU_INTEL_H
