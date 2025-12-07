#include "retryix_bridge_intel_l0.h"
#include <string.h>

// 動態載入 ze_loader，簡易範例（實際需補全能力偵測）

RETRYIX_API retryix_result_t RETRYIX_CALL
retryix_discover_intel_l0_platforms(
  retryix_platform_t* platforms,
  int max_platforms,
  int* platform_count)
{
  // 假設無 Intel Level Zero，回傳 0
  if (platform_count) *platform_count = 0;
  return RETRYIX_SUCCESS;
}

RETRYIX_API int RETRYIX_CALL
retryix_intel_l0_alloc_shared(size_t bytes, size_t alignment, void** out_ptr)
{
  // 範例：不支援 USM，直接回傳失敗
  if (out_ptr) *out_ptr = NULL;
  return -1;
}
